#include "analysis/CFGBuilder.h"

namespace bblift {

namespace {

clang::CFG::BuildOptions makeOptions() {
  clang::CFG::BuildOptions Options;
  Options.PruneTriviallyFalseEdges = false;
  Options.AddEHEdges = false;
  Options.AddInitializers = true;
  Options.AddImplicitDtors = false;
  Options.AddLifetime = false;
  Options.AddLoopExit = false;
  Options.AddTemporaryDtors = false;
  Options.AddScopes = false;
  return Options;
}

} // namespace

std::unique_ptr<clang::CFG> buildCFG(const clang::FunctionDecl *FD,
                                     clang::ASTContext &Ctx) {
  return clang::CFG::buildCFG(FD, FD->getBody(), &Ctx, makeOptions());
}

} // namespace bblift
