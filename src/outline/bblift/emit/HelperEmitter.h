#ifndef BBLIFT_EMIT_HELPER_EMITTER_H
#define BBLIFT_EMIT_HELPER_EMITTER_H

#include "emit/ExpressionRewriter.h"
#include "emit/NameGenerator.h"
#include "model/FunctionModel.h"

#include <optional>
#include <string>

namespace bblift {

// Emits the generated support region: the program-counter enum, the frame
// struct, and one helper per emitted block (spec sections 8, 13, 22).
std::optional<std::string> emitSupport(const FunctionModel &Model,
                                       const GeneratedNames &Names,
                                       const RewriteContext &Ctx);

// Emits the dispatcher body that replaces the original function body (spec
// sections 22 and 30).
std::optional<std::string> emitDispatcherBody(const FunctionModel &Model,
                                              const GeneratedNames &Names,
                                              const RewriteContext &Ctx);

} // namespace bblift

#endif // BBLIFT_EMIT_HELPER_EMITTER_H
