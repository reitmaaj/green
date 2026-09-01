#ifndef GREEN_DRIVER_FIX_H
#define GREEN_DRIVER_FIX_H

#include <string>
#include <vector>

namespace green
{

// Apply only mechanically semantics-preserving fix-its: formatting, missing
// braces, discarded-result postfix->prefix, and NULL spelling. Returns an
// exit status.
int runFix(const std::string &Format, const std::string &Tidy,
           const std::string &Plugin, const std::string &Profile,
           const std::string &CompileDBDir, const std::string &TidyConfig,
           const std::vector<std::string> &Files);

} // namespace green

#endif // GREEN_DRIVER_FIX_H
