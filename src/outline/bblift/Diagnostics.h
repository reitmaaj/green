#ifndef BBLIFT_DIAGNOSTICS_H
#define BBLIFT_DIAGNOSTICS_H

#include <clang/Basic/SourceLocation.h>

#include <optional>
#include <string>

namespace bblift {

enum class SkipReason {
  None,
  UnsupportedLanguage,
  UnsupportedCFGElement,
  UnsupportedControlFlow,
  UnsupportedLifetime,
  MacroBoundary,
  InvalidSourceRange,
  ReplacementConflict,
  PostRewriteParseFailure,
  PostRewriteCompileFailure,
  InternalInvariant,
};

struct Diagnostic {
  SkipReason reason = SkipReason::None;
  std::string message;
  clang::SourceLocation location;
};

using OptDiagnostic = std::optional<Diagnostic>;

} // namespace bblift

#endif // BBLIFT_DIAGNOSTICS_H
