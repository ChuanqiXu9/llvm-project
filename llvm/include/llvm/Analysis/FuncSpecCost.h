//===- FuncSpecCost.h - Cost analysis for function specialization -*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===//
//
// This file implements heuristics for function specialization decisions.
//
//===------------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_FUNCSPECCOST_H
#define LLVM_ANALYSIS_FUNCSPECCOST_H

#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/InstructionCost.h"

namespace llvm {

class AssumptionCache;
class TargetTransformInfo;
class TargetLibraryInfo;
class ArgUsage;

/// Cost/Bonus information for specialize a function with
/// each argument.
class FuncSpecCostInfo {
  /// The cost to specialize the function.
  InstructionCost Cost;
  /// Map from the number of the argument, to the base bonuss for specialize it.
  /// Base bonus stands for the bonus we could get by the function body.
  MapVector<unsigned, unsigned> SpecBonusBaseMap;

  unsigned getBonusBase(Argument *Arg) const;
  unsigned getBonusBase(unsigned Index) const;

public:
  InstructionCost getCost() const { return Cost; }
  unsigned
  getBonus(Argument *Arg, Constant *C,
           function_ref<TargetTransformInfo &(Function &)> GetTTI,
           function_ref<AssumptionCache &(Function &)> GetAC,
           function_ref<const TargetLibraryInfo &(Function &)> GetTLI) const;

  /// Given an ArgUsage, estimating if we should import corresponding function.
  /// We should **only** call this when importing.
  ///
  /// If there is one argument marks function, we would think it would be
  /// inlined if its lines of codes is less than a specific threshold.
  ///
  /// TOOD: Add profiling infomation.
  bool shouldImport(const ArgUsage &) const;

  ArrayRef<std::pair<unsigned, unsigned>> getSpecBonusBaseMap() const {
    return  makeArrayRef(&SpecBonusBaseMap.front(),
                         SpecBonusBaseMap.size());
  }

  FuncSpecCostInfo() {}
  FuncSpecCostInfo(FuncSpecCostInfo &&Other)
      : FuncSpecCostInfo(std::move(Other.Cost),
                         std::move(Other.SpecBonusBaseMap)) {}
  FuncSpecCostInfo(InstructionCost Cost,
                   MapVector<unsigned, unsigned> &&BonusBaseMap)
      : Cost(std::move(Cost)) {
    SpecBonusBaseMap.swap(BonusBaseMap);
  }
};

/// Analysis pass which computes a \c FuncSpecCostInfo.
class FunctionSpecializationAnalysis
    : public AnalysisInfoMixin<FunctionSpecializationAnalysis> {
  friend AnalysisInfoMixin<FunctionSpecializationAnalysis>;
  static AnalysisKey Key;

public:
  using Result = FuncSpecCostInfo;

  Result run(Function &F, FunctionAnalysisManager &);
};

/// Legacy analysis pass which computes a \c FuncSpecCostInfo.
class FunctionSpecializationWrapperPass : public FunctionPass {
  Optional<FuncSpecCostInfo> Info;

public:
  static char ID;

  FunctionSpecializationWrapperPass();

  FuncSpecCostInfo &getFuncSpecCost() { return *Info; }
  const FuncSpecCostInfo &getFuncSpecCost() const { return *Info; }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  StringRef getPassName() const override;
};

/// Represent the usage of args at the callsite used in module summary.
/// We should keep it as small as possible.
///
/// Now we mainly cares if the argument is a function. If yes, it implies
/// a chance to hoist an indirect call to a direct call by function specialize
/// pass.
///
/// It should be easy to add value infomation about the constantness or value
/// range.
class ArgUsage {
private:
  static unsigned ConstantMarker;
  /// Map from ArgNo to the lines of codes if the corresponding argument refer
  /// to a function. If the corresponding argument is a constant other than
  /// function, we would set the value to ConstantMarker(0xffffffff).
  ///
  /// For example, the value of LinesOfArgs for following example:
  /// ```
  ///     foo(var, 1, bar); // bar is a function; var is a variable
  /// ```
  /// should be `[<1, 0xffffffff>, <2, lines of bar>]`.
  SmallVector<std::pair<unsigned, unsigned>, 4> LinesOfArgs;

  friend class FuncSpecCostInfo;
  friend class CalleeInfo;

public:
  ArgUsage() {}
  ArgUsage(const CallBase &);
  ArgUsage(SmallVectorImpl<std::pair<unsigned, unsigned>> &&Uses)
      : LinesOfArgs(std::move(Uses)) {}

  ArgUsage(const ArgUsage &AU) : LinesOfArgs(AU.LinesOfArgs) {}
  ArgUsage(ArgUsage &&AU) : LinesOfArgs(std::move(AU.LinesOfArgs)) {}
  ArgUsage &operator=(ArgUsage &&AU) {
    LinesOfArgs = std::move(AU.LinesOfArgs);
    return *this;
  }
  ArgUsage &operator=(const ArgUsage &AU) {
    LinesOfArgs = AU.LinesOfArgs;
    return *this;
  }

  static bool isConstant(unsigned value) { return value == ConstantMarker; }
};
} // namespace llvm

#endif
