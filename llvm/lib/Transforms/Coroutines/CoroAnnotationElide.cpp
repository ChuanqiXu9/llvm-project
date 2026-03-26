//===- CoroAnnotationElide.cpp - Elide attributed safe coroutine calls ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// \file
// This pass transforms all Call or Invoke instructions that are annotated
// "coro_elide_safe" to call the `.noalloc` variant of coroutine instead.
// The frame of the callee coroutine is allocated inside the caller. A pointer
// to the allocated frame will be passed into the `.noalloc` ramp function.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Coroutines/CoroAnnotationElide.h"
#include "llvm/Transforms/Coroutines/CoroInstr.h"

#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/BranchProbability.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/Utils/CallGraphUpdater.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <cassert>

using namespace llvm;

#define DEBUG_TYPE "coro-annotation-elide"

static cl::opt<float> CoroElideBranchRatio(
    "coro-elide-branch-ratio", cl::init(0.55), cl::Hidden,
    cl::desc("Minimum BranchProbability to consider a elide a coroutine."));
extern cl::opt<unsigned> MinBlockCounterExecution;

static Instruction *getFirstNonAllocaInTheEntryBlock(Function *F) {
  for (Instruction &I : F->getEntryBlock())
    if (!isa<AllocaInst>(&I))
      return &I;
  llvm_unreachable("no terminator in the entry block");
}

// Create an alloca in the caller, using FrameSize and FrameAlign as the callee
// coroutine's activation frame.
static Value *allocateFrameInCallerByAlloca(Function *Caller,
                                            uint64_t FrameSize,
                                            Align FrameAlign) {
  LLVMContext &C = Caller->getContext();
  BasicBlock::iterator InsertPt =
      getFirstNonAllocaInTheEntryBlock(Caller)->getIterator();
  const DataLayout &DL = Caller->getDataLayout();
  auto FrameTy = ArrayType::get(Type::getInt8Ty(C), FrameSize);
  auto *Frame = new AllocaInst(FrameTy, DL.getAllocaAddrSpace(), "", InsertPt);
  Frame->setAlignment(FrameAlign);
  return Frame;
}

static Value *allocateFrameInCallerByStackedAllocation(CallBase *CB,
                                                       Function *Caller,
                                                       uint64_t FrameSize,
                                                       Align FrameAlign) {
  CoroStackedAllocatorInst *StackedAlloc = nullptr;
  for (Instruction &I : instructions(*Caller))
    if (auto *S = dyn_cast<CoroStackedAllocatorInst>(&I)) {
      StackedAlloc = S;
      break;
    }

  if (!StackedAlloc)
    return nullptr;

  // Use the lifetime.end intrinsic to find end of the lifetime of the targetted
  // object.

  llvm::Value *Object = nullptr;
  if (CB->getType()->isVoidTy()) {
    if (!CB->hasStructRetAttr())
      return nullptr;

    Object = CB->getArgOperand(0);
  } else
    Object = CB;

  // Use lifetime information to indicate the lifetime range of the object.
  DominatorTree Dom(*Caller);
  llvm::Instruction *LifetimeStart = nullptr;
  llvm::SmallVector<llvm::Instruction *> LifetimeEnds;

  for (User *U : Object->users()) {
    auto *II = dyn_cast<IntrinsicInst>(U);
    if (!II || II->getIntrinsicID() != Intrinsic::lifetime_start)
      continue;

    if (Dom.dominates(II, CB)) {
      // There are multiple lifetime start that dominates the call.
      // Complex case. Giveup.
      if (LifetimeStart)
        return nullptr;

      LifetimeStart = II;
    }
  }

  if (!LifetimeStart)
    return nullptr;

  for (User *U : Object->users()) {
    auto *II = dyn_cast<IntrinsicInst>(U);

    if (!II || II->getIntrinsicID() != Intrinsic::lifetime_end ||
        !Dom.dominates(LifetimeStart, II))
      continue;

    LifetimeEnds.push_back(II);
  }

  if (LifetimeEnds.empty())
    return nullptr;

  Value *Promise = StackedAlloc->getPromise();
  Function *Alloc = StackedAlloc->getAllocator();
  Function *Dealloc = StackedAlloc->getDeallocator();
  assert(Alloc);
  assert(Dealloc);
  assert(Alloc->arg_size() == 3);
  assert(Dealloc->arg_size() == 3);

  auto Size = ConstantInt::get(Alloc->getArg(1)->getType(), FrameSize);
  auto Align =
      ConstantInt::get(Alloc->getArg(2)->getType(), FrameAlign.value());
  CallInst *Frame = CallInst::Create(Alloc->getFunctionType(), Alloc,
                                     {Promise, Size, Align}, "",
                                     LifetimeStart->getIterator());
  auto DebugLoc = CB->getDebugLoc();
  Frame->setDebugLoc(DebugLoc);

  for (auto *LifetimeEnd : LifetimeEnds) {
    auto *DeallocInst = CallInst::Create(Dealloc->getFunctionType(), Dealloc,
                                         {Promise, Frame, Align}, "",
                                         LifetimeEnd->getNextNode()->getIterator());
    DeallocInst->setDebugLoc(DebugLoc);
  }

  return Frame;
}

// Given a call or invoke instruction to the elide safe coroutine, this function
// does the following:
//  - Replace the old CB with a new Call or Invoke to `NewCallee`, with the
//    pointer to the frame as an additional argument to NewCallee.
static void processCall(CallBase *CB, Function *Caller, Function *NewCallee,
                        Value *FramePtr) {
  auto NewCBInsertPt = CB->getIterator();
  llvm::CallBase *NewCB = nullptr;
  SmallVector<Value *, 4> NewArgs;
  NewArgs.append(CB->arg_begin(), CB->arg_end());
  NewArgs.push_back(FramePtr);

  if (auto *CI = dyn_cast<CallInst>(CB)) {
    auto *NewCI = CallInst::Create(NewCallee->getFunctionType(), NewCallee,
                                   NewArgs, "", NewCBInsertPt);
    NewCI->setTailCallKind(CI->getTailCallKind());
    NewCB = NewCI;
  } else if (auto *II = dyn_cast<InvokeInst>(CB)) {
    NewCB = InvokeInst::Create(NewCallee->getFunctionType(), NewCallee,
                               II->getNormalDest(), II->getUnwindDest(),
                               NewArgs, {}, "", NewCBInsertPt);
  } else {
    llvm_unreachable("CallBase should either be Call or Invoke!");
  }

  NewCB->setCalledFunction(NewCallee->getFunctionType(), NewCallee);
  NewCB->setCallingConv(CB->getCallingConv());
  NewCB->setAttributes(CB->getAttributes());
  NewCB->setDebugLoc(CB->getDebugLoc());
  std::copy(CB->bundle_op_info_begin(), CB->bundle_op_info_end(),
            NewCB->bundle_op_info_begin());

  NewCB->removeFnAttr(llvm::Attribute::CoroElideSafe);
  CB->replaceAllUsesWith(NewCB);

  InlineFunctionInfo IFI;
  InlineResult IR = InlineFunction(*NewCB, IFI);
  if (IR.isSuccess()) {
    CB->eraseFromParent();
  } else {
    NewCB->replaceAllUsesWith(CB);
    NewCB->eraseFromParent();
  }
}

PreservedAnalyses CoroAnnotationElidePass::run(LazyCallGraph::SCC &C,
                                               CGSCCAnalysisManager &AM,
                                               LazyCallGraph &CG,
                                               CGSCCUpdateResult &UR) {
  bool Changed = false;
  CallGraphUpdater CGUpdater;
  CGUpdater.initialize(CG, C, AM, UR);

  auto &FAM =
      AM.getResult<FunctionAnalysisManagerCGSCCProxy>(C, CG).getManager();

  for (LazyCallGraph::Node &N : C) {
    Function *Callee = &N.getFunction();
    Function *NewCallee = Callee->getParent()->getFunction(
        (Callee->getName() + ".noalloc").str());
    if (!NewCallee)
      continue;

    SmallVector<CallBase *, 4> Users;
    for (auto *U : Callee->users()) {
      if (auto *CB = dyn_cast<CallBase>(U)) {
        if (CB->getCalledFunction() == Callee)
          Users.push_back(CB);
      }
    }
    auto FramePtrArgPosition = NewCallee->arg_size() - 1;
    auto FrameSize =
        NewCallee->getParamDereferenceableBytes(FramePtrArgPosition);
    auto FrameAlign =
        NewCallee->getParamAlign(FramePtrArgPosition).valueOrOne();

    auto &ORE = FAM.getResult<OptimizationRemarkEmitterAnalysis>(*Callee);

    for (auto *CB : Users) {
      auto *Caller = CB->getFunction();
      if (!Caller)
        continue;

      bool IsCallerPresplitCoroutine = Caller->isPresplitCoroutine();
      bool HasAttr = CB->hasFnAttr(llvm::Attribute::CoroElideSafe);
      if (IsCallerPresplitCoroutine && HasAttr) {
        auto &BFI = FAM.getResult<BlockFrequencyAnalysis>(*Caller);

        auto BlockFreq = BFI.getBlockFreq(CB->getParent()).getFrequency();
        auto EntryFreq = BFI.getEntryFreq().getFrequency();
        uint64_t MinFreq =
            static_cast<uint64_t>(EntryFreq * CoroElideBranchRatio);

        if (BlockFreq < MinFreq) {
          ORE.emit([&]() {
            return OptimizationRemarkMissed(
                       DEBUG_TYPE, "CoroAnnotationElideUnlikely", Caller)
                   << "'" << ore::NV("callee", Callee->getName())
                   << "' not elided in '"
                   << ore::NV("caller", Caller->getName())
                   << "' because of low frequency: "
                   << ore::NV("block_freq", BlockFreq)
                   << " (threshold: " << ore::NV("min_freq", MinFreq) << ")";
          });
          continue;
        }

        auto *CallerN = CG.lookup(*Caller);
        auto *CallerC = CallerN ? CG.lookupSCC(*CallerN) : nullptr;
        // If CallerC is nullptr, it means LazyCallGraph hasn't visited Caller
        // yet. Skip the call graph update.
        auto ShouldUpdateCallGraph = !!CallerC;

        Value *FramePtr = allocateFrameInCallerByStackedAllocation(
            CB, Caller, FrameSize, FrameAlign);
        if (!FramePtr)
          FramePtr = allocateFrameInCallerByAlloca(Caller, FrameSize,
                                                   FrameAlign);

        processCall(CB, Caller, NewCallee, FramePtr);

        ORE.emit([&]() {
          return OptimizationRemark(DEBUG_TYPE, "CoroAnnotationElide", Caller)
                 << "'" << ore::NV("callee", Callee->getName())
                 << "' elided in '" << ore::NV("caller", Caller->getName())
                 << "' (block_freq: " << ore::NV("block_freq", BlockFreq)
                 << ")";
        });

        FAM.invalidate(*Caller, PreservedAnalyses::none());
        Changed = true;
        if (ShouldUpdateCallGraph)
          updateCGAndAnalysisManagerForCGSCCPass(CG, *CallerC, *CallerN, AM, UR,
                                                 FAM);

      } else {
        ORE.emit([&]() {
          return OptimizationRemarkMissed(DEBUG_TYPE, "CoroAnnotationElide",
                                          Caller)
                 << "'" << ore::NV("callee", Callee->getName())
                 << "' not elided in '" << ore::NV("caller", Caller->getName())
                 << "' (caller_presplit="
                 << ore::NV("caller_presplit", IsCallerPresplitCoroutine)
                 << ", elide_safe_attr=" << ore::NV("elide_safe_attr", HasAttr)
                 << ")";
        });
      }
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
