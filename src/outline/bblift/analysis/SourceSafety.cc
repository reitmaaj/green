#include "analysis/SourceSafety.h"

#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

namespace bblift {

namespace {

bool isMacroExpansion(clang::SourceLocation Loc,
                      const clang::SourceManager &SM) {
  if (Loc.isInvalid()) {
    return true;
  }
  return SM.isMacroArgExpansion(Loc) || SM.isMacroBodyExpansion(Loc);
}

} // namespace

OptDiagnostic checkSourceRange(const clang::CharSourceRange &Range,
                               const clang::SourceManager &SM,
                               const clang::LangOptions &LO,
                               const Options &Opts) {
  (void)LO;
  clang::SourceLocation Begin = Range.getBegin();
  clang::SourceLocation End = Range.getEnd();
  if (isMacroExpansion(Begin, SM) || isMacroExpansion(End, SM)) {
    return Diagnostic{SkipReason::MacroBoundary,
                      "rewrite would enter a macro expansion", Begin};
  }
  if (Begin.isInvalid() || End.isInvalid()) {
    return Diagnostic{SkipReason::InvalidSourceRange, "invalid source range",
                      Begin};
  }
  if (SM.isInSystemHeader(Begin) || SM.isInSystemHeader(End)) {
    return Diagnostic{SkipReason::InvalidSourceRange,
                      "rewrite targets a system header", Begin};
  }
  if (!Opts.headers && !SM.isWrittenInMainFile(Begin)) {
    return Diagnostic{SkipReason::InvalidSourceRange,
                      "rewrite targets a non-main file", Begin};
  }
  return std::nullopt;
}

} // namespace bblift
