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
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/InstructionCost.h"

namespace llvm {

class AssumptionCache;
class TargetTransformInfo;
class TargetLibraryInfo;

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

} // namespace llvm

#endif
