#include "emit/TypePrinter.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>

namespace bblift {

std::string printType(clang::QualType T, const clang::ASTContext &Ctx) {
  clang::PrintingPolicy Policy(Ctx.getLangOpts());
  Policy.SuppressTagKeyword = true;
  Policy.SuppressUnwrittenScope = true;
  Policy.PolishForDeclaration = true;
  return T.getAsString(Policy);
}

} // namespace bblift
