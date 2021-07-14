//===- FuncSpecCost.cpp - Cost analysis for function specializer
//-*---------===//
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===-----------------------------------------------------------------------===//
//
// This file implements heuristics for specializing function decisions.
//
//===-----------------------------------------------------------------------===//

#include "llvm/Analysis/FuncSpecCost.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/CommandLine.h"
#include <cassert>
#include <cmath>

using namespace llvm;

#define DEBUG_TYPE "func-spec-cost"

/// Limit on instruction count of imported functions.
/// The function whose number of instruction below this argument
/// is considered to be inlined during function importing.
static cl::opt<unsigned> PotentialInlingLimit(
    "funcspec-potential-inlininglimit", cl::init(100), cl::Hidden,
    cl::value_desc("N"),
    cl::desc("Only functions with less than N instructions are considered to "
             "be inlined during analysis of func spec."));

static cl::opt<double>
    BonusFactorFromInlining("func-spec-bonus-from-inlining-factor", cl::Hidden,
                            cl::desc("Factor for bonus from potetial inling."),
                            cl::init(10));

static cl::opt<unsigned>
    AvgLoopIterationCount("func-specialization-avg-iters-cost", cl::Hidden,
                          cl::desc("Average loop iteration count cost"),
                          cl::init(10));

static Function *getFunction(Value *CalledValue) {
  // Since the argument is a function pointer, its incoming constant values
  // should be functions or constant expressions. The code below attempts to
  // look through cast expressions to find the function that will be called.
  if (!isa<PointerType>(CalledValue->getType()) ||
      !isa<FunctionType>(CalledValue->getType()->getPointerElementType()))
    return nullptr;

  while (isa<ConstantExpr>(CalledValue) &&
         cast<ConstantExpr>(CalledValue)->isCast())
    CalledValue = cast<User>(CalledValue)->getOperand(0);
  return dyn_cast<Function>(CalledValue);
}

/// Bonus equals to basebonus plus extrabonus.
/// The extrabonus means bonus we could find at the callsites.
/// For example, if the callsite passes a function to the specializing function,
/// we could compute the bonus for the callsite by the bonus of inline
/// the function into the specializing function.
unsigned FuncSpecCostInfo::getBonus(
    Argument *Arg, Constant *C,
    function_ref<TargetTransformInfo &(Function &)> GetTTI,
    function_ref<AssumptionCache &(Function &)> GetAC,
    function_ref<const TargetLibraryInfo &(Function &)> GetTLI) const {
  LLVM_DEBUG(dbgs() << "FnSpecialization: Analysing bonus for: " << *Arg
                    << "\n");

  unsigned BonusBase = getBonusBase(Arg);

  // The below heuristic is only concerned with exposing inlining
  // opportunities via indirect call promotion. If the argument is not a
  // function pointer, give up.
  Function *CalledFunction = getFunction(C);
  if (!CalledFunction)
    return BonusBase;

  // Get TTI for the called function (used for the inline cost).
  auto &CalleeTTI = (GetTTI)(*CalledFunction);

  int Bonus = 0;
  for (User *U : Arg->users()) {
    if (!isa<CallInst>(U) && !isa<InvokeInst>(U))
      continue;
    auto *CS = cast<CallBase>(U);
    if (CS->getCalledOperand() != Arg)
      continue;

    // Get the cost of inlining the called function at this call site. Note
    // that this is only an estimate. The called function may eventually
    // change in a way that leads to it not being inlined here, even though
    // inlining looks profitable now. For example, one of its called
    // functions may be inlined into it, making the called function too large
    // to be inlined into this call site.
    //
    // We apply a boost for performing indirect call promotion by increasing
    // the default threshold by the threshold for indirect calls.
    auto Params = getInlineParams();
    Params.DefaultThreshold += InlineConstants::IndirectCallThreshold;
    InlineCost IC =
        getInlineCost(*CS, CalledFunction, Params, CalleeTTI, GetAC, GetTLI);

    // We clamp the bonus for this call to be between zero and the default
    // threshold.
    if (IC.isAlways())
      Bonus += Params.DefaultThreshold;
    else if (IC.isVariable() && IC.getCostDelta() > 0)
      Bonus += IC.getCostDelta();
  }

  return BonusBase + Bonus;
}

unsigned FuncSpecCostInfo::getBonusBase(Argument *Arg) const {
  return getBonusBase(Arg->getArgNo());
}

unsigned FuncSpecCostInfo::getBonusBase(unsigned Index) const {
  if (!SpecBonusBaseMap.count(Index))
    return 0;
  return SpecBonusBaseMap.lookup(Index);
}

static InstructionCost getSpecializationCost(Function &F, AssumptionCache &AC,
                                             TargetTransformInfo &TTI) {
  // Compute the code metrics for the function.
  SmallPtrSet<const Value *, 32> EphValues;
  CodeMetrics::collectEphemeralValues(&F, &AC, EphValues);
  CodeMetrics Metrics;
  for (BasicBlock &BB : F)
    Metrics.analyzeBasicBlock(&BB, TTI, EphValues);

  // If the code metrics reveal that we shouldn't duplicate the function, we
  // shouldn't specialize it. Set the specialization cost to Invalid.
  if (Metrics.notDuplicatable) {
    InstructionCost C{};
    C.setInvalid();
    return C;
  }

  return Metrics.NumInsts * InlineConstants::InstrCost;
}

static InstructionCost getUserBonus(User *U, TargetTransformInfo &TTI,
                                    LoopInfo &LI) {
  auto *I = dyn_cast_or_null<Instruction>(U);
  // If not an instruction we do not know how to evaluate.
  // Keep minimum possible cost for now so that it doesnt affect
  // specialization.
  if (!I)
    return std::numeric_limits<unsigned>::min();

  InstructionCost Cost =
      TTI.getUserCost(U, TargetTransformInfo::TCK_SizeAndLatency);

  // Traverse recursively if there are more uses.
  // TODO: Any other instructions to be added here?
  if (I->mayReadFromMemory() || I->isCast())
    for (auto *User : I->users())
      Cost += getUserBonus(User, TTI, LI);

  // Increase the cost if it is inside the loop.
  unsigned LoopDepth = LI.getLoopDepth(I->getParent());
  Cost *= std::pow((long double)AvgLoopIterationCount, LoopDepth);
  return Cost;
}

static FuncSpecCostInfo CreateFuncSpecCostInfo(Function &F, AssumptionCache &AC,
                                               TargetTransformInfo &TTI,
                                               LoopInfo &LI) {
  InstructionCost SpecializeCost = getSpecializationCost(F, AC, TTI);
  MapVector<unsigned, unsigned> SpecBonusBaseMap;
  for (Argument &A : F.args()) {
    InstructionCost Cost = 0;
    for (auto *U : A.users())
      Cost += getUserBonus(U, TTI, LI);

    if (Cost.isValid() && Cost > 0)
      SpecBonusBaseMap.insert({A.getArgNo(), *Cost.getValue()});
  }
  return FuncSpecCostInfo(SpecializeCost, std::move(SpecBonusBaseMap));
}

AnalysisKey FunctionSpecializationAnalysis::Key;

FuncSpecCostInfo
FunctionSpecializationAnalysis::run(Function &F, FunctionAnalysisManager &FAM) {
  TargetTransformInfo &TTI = FAM.getResult<TargetIRAnalysis>(F);
  LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
  AssumptionCache &AC = FAM.getResult<AssumptionAnalysis>(F);
  return CreateFuncSpecCostInfo(F, AC, TTI, LI);
}

char FunctionSpecializationWrapperPass::ID = 0;
INITIALIZE_PASS_BEGIN(FunctionSpecializationWrapperPass, "func-spec-cost",
                      "Function Specialization Cost Analysis", false, true)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(FunctionSpecializationWrapperPass, "func-spec-cost",
                    "Function Specialization Cost Analysis", false, true)

FunctionSpecializationWrapperPass::FunctionSpecializationWrapperPass()
    : FunctionPass(ID) {
  initializeFunctionSpecializationWrapperPassPass(
      *PassRegistry::getPassRegistry());
}

void FunctionSpecializationWrapperPass::getAnalysisUsage(
    AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
}

bool FunctionSpecializationWrapperPass::runOnFunction(Function &F) {
  TargetTransformInfo &TTI =
      getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  AssumptionCache &AC =
      getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
  Info.emplace(CreateFuncSpecCostInfo(F, AC, TTI, LI));
  return false;
}

StringRef FunctionSpecializationWrapperPass::getPassName() const {
  return "Function Specialization Cost Analysis";
}

unsigned ArgUsage::ConstantMarker = ~(unsigned)0;

ArgUsage::ArgUsage(const CallBase &CB) {
  for (auto &U : CB.args()) {
    Function *F = getFunction(U.get());
    LinesOfArgs.push_back({CB.getArgOperandNo(&U), ConstantMarker});
    if (!F)
      continue;

    LinesOfArgs.push_back({CB.getArgOperandNo(&U), F->getInstructionCount()});
  }
}

bool FuncSpecCostInfo::shouldImport(const ArgUsage &AU) const {
  if (!Cost.isValid())
    return false;
  unsigned SpecCost = *Cost.getValue();
  /// FIXME: It would consider the first argument who fits
  /// the condition. It should be fixed after Function Specialization
  /// pass fix this.
  for (auto &IndexValuePair : AU.LinesOfArgs) {
    unsigned Index = IndexValuePair.first;
    unsigned Value = IndexValuePair.second;
    unsigned BonusBase = getBonusBase(Index);
    // Should we consider attributes like `noinline` and `always_inline` here?
    if (!ArgUsage::isConstant(Value) && Value < PotentialInlingLimit)
      BonusBase += BonusFactorFromInlining * (PotentialInlingLimit - Value);

    if (BonusBase > SpecCost)
      return true;
  }
  return false;
}