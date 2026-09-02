#include "emit/ReplacementBuilder.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/Tooling/Core/Replacement.h>

namespace bblift {

OptDiagnostic buildFunctionEdits(const clang::FunctionDecl *FD,
                                 const FunctionModel &Model,
                                 const GeneratedNames &Names,
                                 const std::string &Support,
                                 const std::string &Dispatcher,
                                 const clang::SourceManager &SM,
                                 clang::tooling::Replacements &Out) {
  (void)Model;
  (void)Names;

  clang::SourceLocation BodyStart = FD->getBody()->getBeginLoc();
  clang::SourceLocation BodyEnd = FD->getBody()->getEndLoc();
  if (BodyStart.isInvalid() || BodyEnd.isInvalid()) {
    return Diagnostic{SkipReason::InvalidSourceRange,
                      "function body has an invalid source range",
                      FD->getLocation()};
  }

  unsigned BodyOffset = SM.getFileOffset(BodyStart);
  unsigned BodyLength = SM.getFileOffset(BodyEnd) - BodyOffset + 1;

  clang::tooling::Replacement Insert(SM, FD->getBeginLoc(), 0, Support);
  clang::tooling::Replacement Body(SM, BodyStart, BodyLength, Dispatcher);

  if (llvm::Error E = Out.add(Insert)) {
    return Diagnostic{SkipReason::ReplacementConflict,
                      "overlapping replacement while inserting support",
                      FD->getLocation()};
  }
  if (llvm::Error E = Out.add(Body)) {
    return Diagnostic{SkipReason::ReplacementConflict,
                      "overlapping replacement for function body",
                      FD->getLocation()};
  }
  return std::nullopt;
}

} // namespace bblift
