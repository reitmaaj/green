#ifndef BBLIFT_FRONTEND_FUNCTION_COLLECTOR_H
#define BBLIFT_FRONTEND_FUNCTION_COLLECTOR_H

#include "Options.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>

#include <vector>

namespace bblift {

// Finds function definitions in the translation unit selected by the CLI
// options (--function / --function-regex / --all-functions). Never selects
// main unless explicitly permitted (spec section 24).
std::vector<const clang::FunctionDecl *>
collectFunctions(clang::ASTContext &Ctx, const Options &Opts);

} // namespace bblift

#endif // BBLIFT_FRONTEND_FUNCTION_COLLECTOR_H
