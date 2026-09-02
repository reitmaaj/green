#ifndef BBLIFT_EMIT_REPLACEMENT_BUILDER_H
#define BBLIFT_EMIT_REPLACEMENT_BUILDER_H

#include "Diagnostics.h"
#include "emit/NameGenerator.h"
#include "model/FunctionModel.h"

#include <clang/Basic/SourceManager.h>
#include <clang/Tooling/Core/Replacement.h>

namespace bblift {

// Builds the non-overlapping edits for one function: generated support is
// inserted immediately before the function and the original body is replaced
// by the dispatcher body (spec section 30).
OptDiagnostic buildFunctionEdits(const clang::FunctionDecl *FD,
                                 const FunctionModel &Model,
                                 const GeneratedNames &Names,
                                 const std::string &Support,
                                 const std::string &Dispatcher,
                                 const clang::SourceManager &SM,
                                 clang::tooling::Replacements &Out);

} // namespace bblift

#endif // BBLIFT_EMIT_REPLACEMENT_BUILDER_H
