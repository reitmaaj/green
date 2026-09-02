#ifndef BBLIFT_ANALYSIS_PREDICATES_H
#define BBLIFT_ANALYSIS_PREDICATES_H

#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Analysis/CFG.h>
#include <llvm/ADT/DenseMap.h>

#include <cstddef>

namespace bblift {

enum class ExitPolarity { True, False, Neutral };

// Clang documents successor ordering for branch terminators as
// [Then, Else] / [Body, Exit] (CFG.h). Polarity is derived from this order,
// which is pinned by golden CFG tests.
inline ExitPolarity successorPolarity(const clang::CFGBlock *B,
                                      size_t SuccIndex) {
  (void)B;
  return SuccIndex == 0 ? ExitPolarity::True : ExitPolarity::False;
}

// Strips any chain of LabelStmt wrappers from S, returning the underlying
// statement.
inline const clang::Stmt *unwrapLabel(const clang::Stmt *S) {
  const clang::Stmt *Cur = S;
  while (const auto *LS = llvm::dyn_cast<clang::LabelStmt>(Cur)) {
    Cur = LS->getSubStmt();
  }
  return Cur;
}

// True when E, used as a branch or switch condition, must be parenthesized in
// the emitted `return cond ? A : B;` because it is a comma, assignment, or
// conditional expression.
inline bool needsParensInCondition(const clang::Expr *E) {
  if (const auto *BO = llvm::dyn_cast<clang::BinaryOperator>(E)) {
    switch (BO->getOpcode()) {
    case clang::BO_Comma:
    case clang::BO_Assign:
    case clang::BO_MulAssign:
    case clang::BO_DivAssign:
    case clang::BO_RemAssign:
    case clang::BO_AddAssign:
    case clang::BO_SubAssign:
    case clang::BO_ShlAssign:
    case clang::BO_ShrAssign:
    case clang::BO_AndAssign:
    case clang::BO_XorAssign:
    case clang::BO_OrAssign:
      return true;
    default:
      break;
    }
  }
  if (llvm::isa<clang::ConditionalOperator>(E) ||
      llvm::isa<clang::BinaryConditionalOperator>(E)) {
    return true;
  }
  return false;
}

// Returns true when S contains a short-circuit (&&, ||) or conditional (?:)
// expression anywhere in its subtree.
inline bool hasBranchingExpr(const clang::Stmt *S) {
  if (const auto *BO = llvm::dyn_cast<clang::BinaryOperator>(S)) {
    if (BO->getOpcode() == clang::BO_LAnd || BO->getOpcode() == clang::BO_LOr) {
      return true;
    }
  }
  if (llvm::isa<clang::ConditionalOperator>(S) ||
      llvm::isa<clang::BinaryConditionalOperator>(S)) {
    return true;
  }
  for (const clang::Stmt *Child : S->children()) {
    if (hasBranchingExpr(Child)) {
      return true;
    }
  }
  return false;
}

// True when T is an eligible scalar type: not a reference, array, record,
// vector, or variably-modified type.
inline bool isScalar(clang::QualType T) {
  if (T->isReferenceType() || T->isArrayType() || T->isRecordType() ||
      T->isVectorType()) {
    return false;
  }
  if (T->isVariablyModifiedType()) {
    return false;
  }
  return true;
}

// Own parent map over a function body. clang::Stmt::getParent() is not
// available; this map is built once and reused for all ancestor queries.
class ParentMap {
public:
  void build(const clang::Stmt *S, const clang::Stmt *P) {
    if (S == nullptr) {
      return;
    }
    Parent[S] = P;
    if (const auto *DS = llvm::dyn_cast<clang::DeclStmt>(S)) {
      for (const clang::Decl *D : DS->decls()) {
        if (const auto *VD = llvm::dyn_cast<clang::VarDecl>(D)) {
          build(VD->getInit(), S);
        }
      }
    }
    for (const clang::Stmt *C : S->children()) {
      build(C, S);
    }
  }

  bool isDescendantOf(const clang::Stmt *S, const clang::Stmt *Ancestor) const {
    if (S == Ancestor) {
      return true;
    }
    const clang::Stmt *Cur = S;
    for (;;) {
      auto It = Parent.find(Cur);
      if (It == Parent.end()) {
        return false;
      }
      if (It->second == Ancestor) {
        return true;
      }
      Cur = It->second;
    }
  }

  const clang::Expr *fullExprRoot(const clang::Expr *E) const {
    const clang::Expr *Cur = E;
    for (;;) {
      auto It = Parent.find(Cur);
      if (It == Parent.end()) {
        return Cur;
      }
      if (const auto *P = llvm::dyn_cast<clang::Expr>(It->second)) {
        Cur = P;
      } else {
        return Cur;
      }
    }
  }

private:
  llvm::DenseMap<const clang::Stmt *, const clang::Stmt *> Parent;
};

} // namespace bblift

#endif // BBLIFT_ANALYSIS_PREDICATES_H
