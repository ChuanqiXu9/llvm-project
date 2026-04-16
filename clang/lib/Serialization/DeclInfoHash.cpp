//===- DeclInfoHash.cpp - DeclInfo serialization helpers ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/TemplateName.h"
#include "clang/AST/TemplateBase.h"
#include "clang/AST/TypeVisitor.h"
#include "clang/AST/ODRHash.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/Module.h"
#include "clang/Serialization/ASTBitCodes.h"
#include "clang/Serialization/ASTWriter.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace clang;
using namespace clang::serialization;

namespace {

class DeclInfoHashVisitor : public ConstDeclVisitor<DeclInfoHashVisitor> {
  typedef ConstDeclVisitor<DeclInfoHashVisitor> Inherited;
  llvm::FoldingSetNodeID ID;
  ODRHash Hasher;
  ASTWriter &Writer;

public:
  explicit DeclInfoHashVisitor(ASTWriter &W) : Writer(W) {}
  
  uint64_t computeStableHash() {
    ID.AddInteger(Hasher.CalculateHash());
    return ID.computeStableHash();
  }

  void VisitFunctionDecl(const FunctionDecl *D) {
    VisitValueDecl(D);
    AddQualType(D->getReturnType());
    ID.AddInteger(D->getNumParams());
    for (auto *Param : D->parameters()) {
      AddQualType(Param->getType());
    }
    ID.AddInteger(D->isVariadic());

    if (const auto *MD = dyn_cast<CXXMethodDecl>(D)) {
      ID.AddInteger(MD->isConst());
      ID.AddInteger(MD->isVolatile());
      ID.AddInteger(MD->isStatic());
      ID.AddInteger(static_cast<unsigned>(MD->getRefQualifier()));
    }
  }
  
  void VisitValueDecl(const ValueDecl *D) {
    VisitDecl(D);
    AddQualType(D->getType());
  }

  void VisitParmVarDecl(const ParmVarDecl *D) {
    VisitValueDecl(D);
    ID.AddInteger(D->getFunctionScopeIndex());
  }
  
  void VisitUsingShadowDecl(const UsingShadowDecl *D) {
    VisitDecl(D);
    
    if (NamedDecl *Target = D->getTargetDecl())
      AddDecl(Target);
  }

  void VisitFunctionTemplateDecl(const FunctionTemplateDecl *D) {
    VisitDecl(D);
    AddDecl(D->getTemplatedDecl());
  }
  
  void VisitClassTemplateDecl(const ClassTemplateDecl *D) {
    VisitDecl(D);
    AddDecl(D->getTemplatedDecl());
  }
  
  void VisitVarTemplateDecl(const VarTemplateDecl *D) {
    VisitDecl(D);
    AddDecl(D->getTemplatedDecl());
  }
  
  void VisitDecl(const Decl *D) {
    AddDeclarationNameIfNamed(D);
    ID.AddInteger(D->getKind());
    AddContextInfo(D);
    
    ID.AddInteger(D->isImplicit());
    
    if (const auto *TD = dyn_cast<TemplateDecl>(D)) {
      AddTemplateParameters(TD->getTemplateParameters());
    }

    AddTemplateSpecializationArgs(D);
    
    if (const Module *M = D->getOwningModule();
        M && M->isNamedModule()) {
      addModuleName(M->getPrimaryModuleInterfaceName());
    }

    Inherited::VisitDecl(D);
  }
  
  void VisitCXXRecordDecl(const CXXRecordDecl *D) {
    VisitDecl(D);
    ID.AddInteger(D->isLambda());
    ID.AddInteger(D->isCompleteDefinition());
    
    if (const auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(D)) {
      for (const auto *Member : CTSD->decls()) {
        if (const auto *FD = dyn_cast<FunctionDecl>(Member)) {
          ID.AddInteger(FD->isDefined());
        }
      }
    }
  }

  void AddQualType(QualType T) {
    Hasher.AddQualType(T);
  }
  void AddType(const Type *T) {
    Hasher.AddType(T);
  }

  void addModuleName(StringRef ModuleName) {
    if (!ModuleName.empty())
      ID.AddString(ModuleName);
  }

private:
  void AddDeclarationNameIfNamed(const Decl *D) {
    const auto *ND = dyn_cast<NamedDecl>(D);
    if (!ND)
      return;
    
    DeclarationName Name = ND->getDeclName();
    ID.AddInteger(Name.getNameKind());
    
    switch (Name.getNameKind()) {
    case DeclarationName::Identifier:
      if (const IdentifierInfo *II = Name.getAsIdentifierInfo())
        ID.AddString(II->getName());
      break;
    case DeclarationName::ObjCZeroArgSelector:
    case DeclarationName::ObjCOneArgSelector:
    case DeclarationName::ObjCMultiArgSelector: {
        Selector S = Name.getObjCSelector();
        if (!S.isNull()) {
            unsigned N = S.getNumArgs();
            ID.AddInteger(N);
            unsigned SlotsToCheck = N > 0 ? N : 1;
            for (unsigned i = 0; i < SlotsToCheck; ++i) {
            if (const IdentifierInfo *II = S.getIdentifierInfoForSlot(i))
              ID.AddString(II->getName());
            }
        }
        break;
    }
    case DeclarationName::CXXOperatorName:
      ID.AddInteger(Name.getCXXOverloadedOperator());
      break;
    case DeclarationName::CXXLiteralOperatorName:
      if (const IdentifierInfo *II = Name.getCXXLiteralIdentifier())
        ID.AddString(II->getName());
      break;
    default:
        break;
    }
  }

  void AddContextInfo(const Decl *D) {
    const DeclContext *Ctx = D->getDeclContext();
    if (!Ctx) return;

    auto *CtxDecl = cast<Decl>(Ctx);
    if (isa<TranslationUnitDecl>(CtxDecl)) return;
    
    AddDecl(CtxDecl);
    
    const Decl *LexicalCtxDecl = cast_or_null<Decl>(D->getLexicalDeclContext());
    if (LexicalCtxDecl && LexicalCtxDecl != CtxDecl)
      AddDecl(LexicalCtxDecl);
  }
  
  void AddDecl(const Decl *D) {
    if (!D) return;
    uint64_t Hash = Writer.computeDeclHash(D);
    ID.AddInteger(Hash);
  }

  void AddTemplateSpecializationArgs(const Decl *D) {
    if (const auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(D)) {
      const auto &Args = CTSD->getTemplateArgs();
      ID.AddInteger(Args.size());
      for (const auto &Arg : Args.asArray()) {
        AddTemplateArgument(Arg);
      }
    }
    if (const auto *VTSD = dyn_cast<VarTemplateSpecializationDecl>(D)) {
      const auto &Args = VTSD->getTemplateArgs();
      ID.AddInteger(Args.size());
      for (const auto &Arg : Args.asArray()) {
        AddTemplateArgument(Arg);
      }
    }
    if (const auto *FD = dyn_cast<FunctionDecl>(D)) {
      if (const auto *SpecArgs = FD->getTemplateSpecializationArgs()) {
        ID.AddInteger(SpecArgs->size());
        for (const auto &Arg : SpecArgs->asArray()) {
          AddTemplateArgument(Arg);
        }
      }
    }
  }

  void AddTemplateArgument(TemplateArgument TA) {
    Hasher.AddTemplateArgument(TA);
  }

  void AddTemplateParameters(const TemplateParameterList *Params) {
    if (!Params) return;
    Hasher.AddTemplateParameterList(Params);
  }
};

} // namespace

uint64_t ASTWriter::computeDeclHash(const Decl *D) {
  if (!D) return 0;

  auto It = DeclHashCache.find(D);
  if (It != DeclHashCache.end())
    return It->second;

  DeclInfoHashVisitor Visitor(*this);
  Visitor.Visit(D);
  uint64_t Hash = Visitor.computeStableHash();
  
  DeclHashCache[D] = Hash;
  return Hash;
}