#ifndef BBLIFT_FRONTEND_ACTION_H
#define BBLIFT_FRONTEND_ACTION_H

#include <clang/Frontend/FrontendAction.h>

namespace bblift {

struct RunContext;

// Frontend action that runs the lifting pipeline over a translation unit.
class LiftingAction : public clang::ASTFrontendAction {
public:
  explicit LiftingAction(RunContext &Context) : Context(Context) {}

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &CI,
                    llvm::StringRef InFile) override;

private:
  RunContext &Context;
};

} // namespace bblift

#endif // BBLIFT_FRONTEND_ACTION_H
