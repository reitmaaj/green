#include "frontend/Action.h"
#include "frontend/Consumer.h"

namespace bblift {

std::unique_ptr<clang::ASTConsumer>
LiftingAction::CreateASTConsumer(clang::CompilerInstance &CI,
                                 llvm::StringRef InFile) {
  (void)InFile;
  return std::make_unique<LiftingConsumer>(CI, Context);
}

} // namespace bblift
