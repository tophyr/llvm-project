//===-- SemaReloc.cpp - Semantic Analysis for Relocation ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements semantic analysis for the P2785R4 `reloc` expression
//  and supporting helpers. The implementation is gated on LangOpts.Relocation
//  (`-frelocation`) at the parser and overload-resolution layers; this file
//  contains the post-parse Sema entry points that the parser dispatches to.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/Sema.h"

using namespace clang;
using namespace sema;

static const DeclRefExpr *getDecomposedObjectRootDeclRef(const Expr *E) {
  E = E->IgnoreParenImpCasts();
  while (const auto *ME = dyn_cast<MemberExpr>(E))
    E = ME->getBase()->IgnoreParenImpCasts();
  while (const auto *DOE = dyn_cast<CXXDecomposedObjectExpr>(E)) {
    E = DOE->getOperand()->IgnoreParenImpCasts();
    while (const auto *ME = dyn_cast<MemberExpr>(E))
      E = ME->getBase()->IgnoreParenImpCasts();
  }
  return dyn_cast<DeclRefExpr>(E);
}

static const NamedDecl *getRelocSubobjectKey(const Expr *E) {
  E = E->IgnoreParenImpCasts();
  if (const auto *DOE = dyn_cast<CXXDecomposedObjectExpr>(E))
    return DOE->isBaseSubobject() ? DOE->getBaseTypeDecl() : nullptr;
  if (const auto *ME = dyn_cast<MemberExpr>(E))
    return dyn_cast<ValueDecl>(ME->getMemberDecl());
  return nullptr;
}

ExprResult Sema::ActOnRelocExpr(Scope *S, SourceLocation Loc, Expr *E) {
  if (DiagnoseUnexpandedParameterPack(E))
    return ExprError();

  if (E->isTypeDependent() || E->isValueDependent())
    return new (Context) CXXRelocExpr(Context.DependentTy, E, Loc);

  auto GetRelocReferenceType = [](Expr *Operand) -> QualType {
    Operand = Operand->IgnoreParens();
    while (const auto *DOE = dyn_cast<CXXDecomposedObjectExpr>(Operand))
      Operand = DOE->getOperand()->IgnoreParens();

    if (const auto *DRE = dyn_cast<DeclRefExpr>(Operand)) {
      if (const auto *VD = dyn_cast<ValueDecl>(DRE->getDecl())) {
        if (const auto *RT = VD->getType()->getAs<ReferenceType>())
          return VD->getType();
      }
    }

    if (const auto *ME = dyn_cast<MemberExpr>(Operand)) {
      if (const auto *VD = dyn_cast<ValueDecl>(ME->getMemberDecl())) {
        if (const auto *RT = VD->getType()->getAs<ReferenceType>())
          return VD->getType();
      }
    }

    return QualType();
  };

  auto SkipRelocSubobjectCasts = [](const Expr *Operand) -> const Expr * {
    Operand = Operand->IgnoreParens();
    while (const auto *DOE = dyn_cast<CXXDecomposedObjectExpr>(Operand))
      Operand = DOE->getOperand()->IgnoreParens();
    while (const auto *CE = dyn_cast<CastExpr>(Operand))
      Operand = CE->getSubExpr()->IgnoreParens();
    return Operand;
  };

  auto FindRelocRootDeclRef =
      [&SkipRelocSubobjectCasts](Expr *Operand) -> const DeclRefExpr * {
    const Expr *Root = SkipRelocSubobjectCasts(Operand);
    while (const auto *ME = dyn_cast<MemberExpr>(Root))
      Root = SkipRelocSubobjectCasts(ME->getBase());
    return dyn_cast<DeclRefExpr>(Root);
  };

  auto IsRelocDecompositionSubobject =
      [](Expr *Operand) -> bool {
    Operand = Operand->IgnoreParens();
    if (const auto *DOE = dyn_cast<CXXDecomposedObjectExpr>(Operand))
      return DOE->isBaseSubobject();
    const Expr *Node = Operand;
    while (const auto *DOE = dyn_cast<CXXDecomposedObjectExpr>(Node))
      Node = DOE->getOperand()->IgnoreParens();
    while (const auto *CE = dyn_cast<CastExpr>(Node))
      Node = CE->getSubExpr()->IgnoreParens();
    return isa<MemberExpr>(Node) || isa<CastExpr>(Operand->IgnoreParens());
  };

  QualType Ty = E->getType();
  if (Ty.isNull() || Ty->isVoidType() || Ty->isFunctionType()) {
    Diag(Loc, diag::err_reloc_operand_not_glvalue) << E->getSourceRange();
    return ExprError();
  }

  Expr::Classification VC = E->Classify(Context);
  if (VC.isPRValue())
    return new (Context) CXXRelocExpr(Ty, E, Loc);

  if (!VC.isGLValue()) {
    Diag(Loc, diag::err_reloc_operand_not_glvalue) << E->getSourceRange();
    return ExprError();
  }

  Expr *Operand = E->IgnoreParens();
  if (const auto *DOE = dyn_cast<CXXDecomposedObjectExpr>(Operand)) {
    if (!DOE->isWholeObject() &&
        (!DOE->isDirectBaseSubobject() || DOE->isVirtualBaseSubobject())) {
      Diag(Loc, diag::err_reloc_operand_not_complete_object)
          << E->getSourceRange();
      return ExprError();
    }
  }
  if (QualType RefTy = GetRelocReferenceType(Operand); !RefTy.isNull()) {
    TypeSourceInfo *TSI = Context.getTrivialTypeSourceInfo(RefTy, Loc);
    return BuildCXXNamedCast(Loc, tok::kw_static_cast, TSI, E,
                             SourceRange(Loc, Loc),
                             SourceRange(Loc, E->getEndLoc()));
  }

  const auto *DRE = FindRelocRootDeclRef(Operand);
  if (DRE && DRE->refersToEnclosingVariableOrCapture()) {
    Diag(Loc, diag::err_reloc_operand_capture) << E->getSourceRange();
    return ExprError();
  }

  const auto *BD = DRE ? dyn_cast<BindingDecl>(DRE->getDecl()) : nullptr;
  if (BD && !BD->isRelocDecompositionBinding()) {
    Diag(Loc, diag::err_reloc_operand_binding) << E->getSourceRange();
    return ExprError();
  }

  const auto *VD = DRE ? dyn_cast<VarDecl>(DRE->getDecl()) : nullptr;
  if (VD && VD->isInitCapture()) {
    Diag(Loc, diag::err_reloc_operand_capture) << E->getSourceRange();
    return ExprError();
  }

  if (IsRelocDecompositionSubobject(Operand)) {
    bool CanRelocateSubobject = false;
    if (const auto *PVD = DRE ? dyn_cast<ParmVarDecl>(DRE->getDecl()) : nullptr)
      CanRelocateSubobject = PVD->isRelocParameter();
    else if (BD)
      CanRelocateSubobject = BD->isRelocDecompositionBinding();
    else if (VD)
      CanRelocateSubobject = VD->isRelocObject();

    if (!CanRelocateSubobject) {
      Diag(Loc, diag::err_reloc_operand_not_complete_object)
          << E->getSourceRange();
      return ExprError();
    }
  } else if ((!VD || !VD->hasLocalStorage()) &&
             !(BD && BD->isRelocDecompositionBinding())) {
    if (const auto *PVD = DRE ? dyn_cast<ParmVarDecl>(DRE->getDecl()) : nullptr) {
      if (PVD->isRelocParameter()) {
        auto *RelocExpr = new (Context) CXXRelocExpr(Ty, E, Loc);
        if (FunctionScopeInfo *FSI = getCurFunction()) {
          FSI->markRelocated(PVD, Loc);
          FSI->recordRelocationEvent(RelocExpr, PVD, Loc);
          if (const auto *RootDRE = getDecomposedObjectRootDeclRef(Operand))
            if (const auto *RootVD = dyn_cast<ValueDecl>(RootDRE->getDecl()))
              if (const auto *Subobject = getRelocSubobjectKey(Operand))
                FSI->recordRelocationEvent(RelocExpr, RootVD, Subobject, Loc),
                FSI->markRelocated(RootVD, Subobject, Loc);
        }
        return RelocExpr;
      }
    }
    Diag(Loc, diag::err_reloc_operand_not_local) << E->getSourceRange();
    return ExprError();
  }

  auto *RelocExpr = new (Context) CXXRelocExpr(Ty, E, Loc);
  if (FunctionScopeInfo *FSI = getCurFunction()) {
    if (BD && BD->isRelocDecompositionBinding()) {
      if (!IsRelocDecompositionSubobject(Operand)) {
        FSI->markRelocated(BD, Loc);
        FSI->recordRelocationEvent(RelocExpr, BD, Loc);
      }
    } else if (!IsRelocDecompositionSubobject(Operand)) {
      FSI->markRelocated(VD, Loc);
      FSI->recordRelocationEvent(RelocExpr, VD, Loc);
    }
    if (const auto *RootDRE = getDecomposedObjectRootDeclRef(Operand))
      if (const auto *RootVD = dyn_cast<ValueDecl>(RootDRE->getDecl()))
        if (const auto *Subobject = getRelocSubobjectKey(Operand))
          FSI->recordRelocationEvent(RelocExpr, RootVD, Subobject, Loc),
          FSI->markRelocated(RootVD, Subobject, Loc);
  }
  return RelocExpr;
}
