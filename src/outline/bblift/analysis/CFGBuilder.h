#ifndef BBLIFT_ANALYSIS_CFGBUILDER_H
#define BBLIFT_ANALYSIS_CFGBUILDER_H

#include <clang/AST/Decl.h>
#include <clang/Analysis/CFG.h>

#include <memory>

namespace bblift {

// Builds a source-oriented clang::CFG for a function definition using the
// pinned build options (spec section 9). Returns nullptr on failure.
std::unique_ptr<clang::CFG> buildCFG(const clang::FunctionDecl *FD,
                                     clang::ASTContext &Ctx);

} // namespace bblift

#endif // BBLIFT_ANALYSIS_CFGBUILDER_H
