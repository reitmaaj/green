#ifndef BBLIFT_FRONTEND_CONSUMER_H
#define BBLIFT_FRONTEND_CONSUMER_H

#include "RunContext.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>

namespace bblift {

class LiftingConsumer : public clang::ASTConsumer {
public:
  LiftingConsumer(clang::CompilerInstance &CI, RunContext &Context)
      : CI(CI), Context(Context) {}

  void HandleTranslationUnit(clang::ASTContext &Ctx) override;

private:
  clang::CompilerInstance &CI;
  RunContext &Context;
};

} // namespace bblift

#endif // BBLIFT_FRONTEND_CONSUMER_H
