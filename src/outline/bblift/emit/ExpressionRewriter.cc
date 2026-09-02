#include "emit/ExpressionRewriter.h"
#include "analysis/VariableAnalysis.h"
#include "emit/TextUtil.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/Support/raw_ostream.h>

namespace bblift {

namespace {

// Collects edits replacing every frame-resident DeclRefExpr under Root with
// `s->field`. Offsets are relative to the statement range base offset.
void collectDeclRefEdits(const clang::Stmt *Root, const FunctionModel &Model,
                         unsigned BaseOffset, const clang::SourceManager &SM,
                         const clang::LangOptions &LO,
                         std::vector<Edit> &Edits) {
  if (Root == nullptr) {
    return;
  }
  if (const auto *DRE = llvm::dyn_cast<clang::DeclRefExpr>(Root)) {
    if (const Local *Field = frameFieldOf(Model, DRE->getDecl())) {
      clang::CharSourceRange Range =
          clang::Lexer::getAsCharRange(DRE->getSourceRange(), SM, LO);
      if (Range.isValid()) {
        unsigned Begin = SM.getFileOffset(Range.getBegin());
        unsigned End = SM.getFileOffset(Range.getEnd());
        Edits.push_back(
            Edit{Begin - BaseOffset, End - Begin, "s->" + Field->field_name});
      }
    }
  }
  for (const clang::Stmt *Child : Root->children()) {
    collectDeclRefEdits(Child, Model, BaseOffset, SM, LO, Edits);
  }
}

std::optional<std::string> rewriteRange(const clang::Stmt *Root,
                                        const clang::CharSourceRange &Range,
                                        const FunctionModel &Model,
                                        const RewriteContext &Ctx) {
  if (Range.isInvalid()) {
    return std::nullopt;
  }
  unsigned Base = Ctx.SM->getFileOffset(Range.getBegin());
  llvm::StringRef Source = clang::Lexer::getSourceText(Range, *Ctx.SM, *Ctx.LO);
  std::vector<Edit> Edits;
  collectDeclRefEdits(Root, Model, Base, *Ctx.SM, *Ctx.LO, Edits);
  return applyEdits(Source, Edits);
}

} // namespace

std::optional<std::string> rewriteStmtText(const clang::Stmt *S,
                                           const FunctionModel &Model,
                                           const RewriteContext &Ctx) {
  clang::CharSourceRange Range =
      clang::Lexer::getAsCharRange(S->getSourceRange(), *Ctx.SM, *Ctx.LO);
  return rewriteRange(S, Range, Model, Ctx);
}

std::optional<std::string> rewriteExprText(const clang::Expr *E,
                                           const FunctionModel &Model,
                                           const RewriteContext &Ctx) {
  clang::CharSourceRange Range =
      clang::Lexer::getAsCharRange(E->getSourceRange(), *Ctx.SM, *Ctx.LO);
  return rewriteRange(E, Range, Model, Ctx);
}

std::optional<std::string> lowerDeclStmt(const clang::DeclStmt *DS,
                                         const FunctionModel &Model,
                                         const RewriteContext &Ctx) {
  std::string Out;
  bool First = true;
  for (const clang::Decl *D : DS->decls()) {
    const auto *VD = llvm::dyn_cast<clang::VarDecl>(D);
    if (VD == nullptr) {
      return std::nullopt;
    }
    const Local *Field = frameFieldOf(Model, VD);
    if (Field == nullptr) {
      return std::nullopt;
    }
    if (VD->getInit() == nullptr) {
      continue;
    }
    std::optional<std::string> Init =
        rewriteExprText(VD->getInit(), Model, Ctx);
    if (!Init) {
      return std::nullopt;
    }
    if (!First) {
      Out.append(" ");
    }
    First = false;
    Out.append("s->" + Field->field_name + " = " + *Init + ";");
  }
  return Out;
}

} // namespace bblift
