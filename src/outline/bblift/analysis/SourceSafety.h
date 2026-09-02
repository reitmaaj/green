#ifndef BBLIFT_ANALYSIS_SOURCESAFETY_H
#define BBLIFT_ANALYSIS_SOURCESAFETY_H

#include "Diagnostics.h"
#include "Options.h"

#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

namespace bblift {

// Checks that a source range is rewriteable: it lives in an owned file and
// never crosses macro or file boundaries (spec sections 20 and 21).
OptDiagnostic checkSourceRange(const clang::CharSourceRange &Range,
                               const clang::SourceManager &SM,
                               const clang::LangOptions &LO,
                               const Options &Opts);

} // namespace bblift

#endif // BBLIFT_ANALYSIS_SOURCESAFETY_H
