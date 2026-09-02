#include "analysis/Eligibility.h"

#include "analysis/Predicates.h"

#include <clang/AST/Attr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>

namespace bblift {

namespace {

const clang::FunctionProtoType *protoType(const clang::FunctionDecl *);

enum bblift_protoType_pc { BBLIFT_PROTOTYPE_B0, BBLIFT_PROTOTYPE_DONE };

struct bblift_protoType_frame {
  const clang::FunctionDecl *p_0_FD;
  const clang::FunctionProtoType *result;
};

static enum bblift_protoType_pc
bblift_protoType_b0(struct bblift_protoType_frame *s) {
  s->result = s->p_0_FD->getType()->getAs<clang::FunctionProtoType>();
  return BBLIFT_PROTOTYPE_DONE;
}

const clang::FunctionProtoType *protoType(const clang::FunctionDecl *FD) {
  struct bblift_protoType_frame s;
  enum bblift_protoType_pc pc;

  s.p_0_FD = FD;
  pc = BBLIFT_PROTOTYPE_B0;

  for (;;) {
    switch (pc) {
    case BBLIFT_PROTOTYPE_B0:
      pc = bblift_protoType_b0(&s);
      break;
    case BBLIFT_PROTOTYPE_DONE:
      return s.result;
    }
  }
}

OptDiagnostic reject(SkipReason Reason, const std::string &Message,
                     clang::SourceLocation Loc) {
  return Diagnostic{Reason, Message, Loc};
}

class FunctionChecker {
public:
  FunctionChecker(const clang::FunctionDecl *FD) : FD(FD) {}

  OptDiagnostic check() {
    if (clang::isa<clang::CXXMethodDecl>(FD)) {
      return reject(SkipReason::UnsupportedLanguage, "member function",
                    FD->getLocation());
    }
    if (isGeneratedHelper(FD)) {
      return reject(SkipReason::UnsupportedLanguage, "generated helper",
                    FD->getLocation());
    }
    if (const auto *PT = protoType(FD)) {
      if (PT->isVariadic()) {
        return reject(SkipReason::UnsupportedLanguage, "variadic function",
                      FD->getLocation());
      }
    } else {
      return reject(SkipReason::UnsupportedLanguage, "K&R function definition",
                    FD->getLocation());
    }
    for (const clang::ParmVarDecl *P : FD->parameters()) {
      if (!isScalar(P->getType())) {
        return reject(SkipReason::UnsupportedLanguage,
                      "non-scalar parameter '" + P->getName().str() + "'",
                      P->getLocation());
      }
    }
    if (OptDiagnostic Result = visit(FD->getBody())) {
      return Result;
    }
    return std::nullopt;
  }

private:
  // The tool's generated helpers are static functions taking a single pointer
  // to the generated frame struct (bblift_*_frame). The name generator
  // guarantees the bblift_ namespace never collides with user identifiers, so
  // this signature uniquely identifies already-lifted helpers (idempotency).
  bool isGeneratedHelper(const clang::FunctionDecl *F) const {
    if (F->getStorageClass() != clang::SC_Static) {
      return false;
    }
    if (F->getNumParams() != 1) {
      return false;
    }
    const clang::QualType T = F->getParamDecl(0)->getType();
    const clang::Type *Pointee = T->getPointeeType().getTypePtrOrNull();
    if (Pointee == nullptr) {
      return false;
    }
    const clang::RecordType *RT = Pointee->getAs<clang::RecordType>();
    if (RT == nullptr) {
      return false;
    }
    const clang::RecordDecl *RD = RT->getDecl();
    if (RD->getIdentifier() == nullptr) {
      return false;
    }
    llvm::StringRef Name = RD->getIdentifier()->getName();
    return Name.starts_with("bblift_") && Name.ends_with("_frame");
  }

  OptDiagnostic checkLocal(const clang::VarDecl *VD) {
    if (VD->getType()->isVariablyModifiedType()) {
      return reject(SkipReason::UnsupportedLifetime,
                    "variable-length array '" + VD->getName().str() + "'",
                    VD->getLocation());
    }
    if (!isScalar(VD->getType())) {
      return reject(SkipReason::UnsupportedLanguage,
                    "non-scalar local '" + VD->getName().str() + "'",
                    VD->getLocation());
    }
    return std::nullopt;
  }

  OptDiagnostic visit(const clang::Stmt *S) {
    if (S == nullptr) {
      return std::nullopt;
    }
    if (const auto *AS = llvm::dyn_cast<clang::AsmStmt>(S)) {
      return reject(SkipReason::UnsupportedLanguage, "inline assembly",
                    AS->getBeginLoc());
    }
    if (llvm::isa<clang::StmtExpr>(S)) {
      return reject(SkipReason::UnsupportedLanguage, "statement expression",
                    S->getBeginLoc());
    }
    if (const auto *IG = llvm::dyn_cast<clang::IndirectGotoStmt>(S)) {
      return reject(SkipReason::UnsupportedControlFlow, "computed goto",
                    IG->getBeginLoc());
    }
    if (const auto *AL = llvm::dyn_cast<clang::AddrLabelExpr>(S)) {
      return reject(SkipReason::UnsupportedControlFlow, "address-of-label",
                    AL->getBeginLoc());
    }
    if (const auto *CE = llvm::dyn_cast<clang::CallExpr>(S)) {
      if (const auto *D = CE->getCalleeDecl()) {
        if (const auto *ND = llvm::dyn_cast<clang::NamedDecl>(D)) {
          llvm::StringRef Name = ND->getName();
          if (Name == "setjmp" || Name == "_setjmp" || Name == "sigsetjmp" ||
              Name == "longjmp" || Name == "_longjmp" || Name == "siglongjmp") {
            return reject(SkipReason::UnsupportedControlFlow, "setjmp/longjmp",
                          CE->getBeginLoc());
          }
        }
      }
    }
    if (const auto *UO = llvm::dyn_cast<clang::UnaryOperator>(S)) {
      if (UO->getOpcode() == clang::UO_AddrOf) {
        const auto *E = UO->getSubExpr()->IgnoreParens();
        if (const auto *DRE = llvm::dyn_cast<clang::DeclRefExpr>(E)) {
          if (llvm::isa<clang::VarDecl>(DRE->getDecl()) ||
              llvm::isa<clang::ParmVarDecl>(DRE->getDecl())) {
            return reject(SkipReason::UnsupportedLifetime,
                          "address-taking of a local", UO->getBeginLoc());
          }
        }
      }
    }
    if (const auto *DS = llvm::dyn_cast<clang::DeclStmt>(S)) {
      for (const clang::Decl *D : DS->decls()) {
        const auto *VD = llvm::dyn_cast<clang::VarDecl>(D);
        if (VD == nullptr) {
          return reject(SkipReason::UnsupportedLanguage,
                        "block-scope non-variable declaration",
                        DS->getBeginLoc());
        }
        if (VD->getStorageClass() == clang::SC_Static) {
          return reject(SkipReason::UnsupportedLifetime,
                        "block-scope static local", VD->getLocation());
        }
        if (VD->getStorageClass() == clang::SC_Extern) {
          return reject(SkipReason::UnsupportedLifetime,
                        "block-scope extern declaration", VD->getLocation());
        }
        if (OptDiagnostic Local = checkLocal(VD)) {
          return Local;
        }
        if (VD->hasAttr<clang::CleanupAttr>()) {
          return reject(SkipReason::UnsupportedLifetime, "cleanup attribute",
                        VD->getLocation());
        }
      }
    }
    for (const clang::Stmt *Child : S->children()) {
      if (OptDiagnostic Result = visit(Child)) {
        return Result;
      }
    }
    return std::nullopt;
  }

  const clang::FunctionDecl *FD;
};

} // namespace

OptDiagnostic checkEligibility(const clang::FunctionDecl *FD) {
  FunctionChecker Checker(FD);
  return Checker.check();
}

} // namespace bblift
