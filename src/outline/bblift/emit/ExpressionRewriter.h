#ifndef BBLIFT_EMIT_EXPRESSION_REWRITER_H
#define BBLIFT_EMIT_EXPRESSION_REWRITER_H

#include "model/FunctionModel.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>

#include <optional>
#include <string>

namespace bblift {

struct RewriteContext {
  const clang::SourceManager *SM = nullptr;
  const clang::LangOptions *LO = nullptr;
  const clang::ASTContext *Ctx = nullptr;
};

// Rewrites the source text of Stmt's token range, replacing every
// DeclRefExpr whose bound declaration is frame-resident with `s->field`.
// All other bytes are preserved verbatim (spec section 29). Returns nullopt
// on an invalid range.
std::optional<std::string> rewriteStmtText(const clang::Stmt *S,
                                           const FunctionModel &Model,
                                           const RewriteContext &Ctx);

// Like rewriteStmtText but for a standalone expression (terminator conditions,
// return values, declaration initializers).
std::optional<std::string> rewriteExprText(const clang::Expr *E,
                                           const FunctionModel &Model,
                                           const RewriteContext &Ctx);

// Lowering for a declaration initializer: produces `s->field = INIT;` when the
// declaration is frame-resident and initialized, and empty text when the
// declaration is uninitialized (spec section 16). Returns nullopt on error.
std::optional<std::string> lowerDeclStmt(const clang::DeclStmt *DS,
                                         const FunctionModel &Model,
                                         const RewriteContext &Ctx);

} // namespace bblift

#endif // BBLIFT_EMIT_EXPRESSION_REWRITER_H
