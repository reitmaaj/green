#ifndef GREEN_DRIVER_OUTLINE_H
#define GREEN_DRIVER_OUTLINE_H

#include <string>
#include <vector>

namespace green
{

// Outline every owned translation unit in Files that still contains
// un-extracted basic blocks: run the vendored green-outline transform, apply
// canonical formatting, then verify the result is green-clean. Files that are
// already outlined are reported and left untouched (idempotent).
//
// Returns EXIT_GREEN when every file is already outlined or was outlined and
// re-verified; EXIT_VIOLATION when a transform produced non-green output.
int runOutline(const std::string &Tidy, const std::string &Plugin,
               const std::string &OutlineBin, const std::string &Format,
               const std::string &Profile, const std::string &CompileDBDir,
               const std::string &TidyConfig,
               const std::vector<std::string> &Files);

} // namespace green

#endif // GREEN_DRIVER_OUTLINE_H
