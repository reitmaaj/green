#ifndef BBLIFT_RUN_CONTEXT_H
#define BBLIFT_RUN_CONTEXT_H

#include "Options.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Tooling/Core/Replacement.h>

#include <map>
#include <string>
#include <vector>

namespace bblift {

struct SkipReport {
  std::string function;
  clang::SourceLocation location;
  std::string reason;
  std::string message;
};

// Mutable state shared between the frontend action and the driver.
struct RunContext {
  const Options *options = nullptr;
  std::map<std::string, clang::tooling::Replacements> replacements;
  std::vector<SkipReport> skips;
};

} // namespace bblift

#endif // BBLIFT_RUN_CONTEXT_H
