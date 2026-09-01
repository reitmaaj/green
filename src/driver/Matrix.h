#ifndef GREEN_DRIVER_MATRIX_H
#define GREEN_DRIVER_MATRIX_H

#include "CompileDB.h"
#include "Config.h"

#include <map>
#include <string>
#include <vector>

namespace green
{

struct CellResult
{
    std::string Label;
    bool Pass = false;
    std::string Output;
};

// Run the four compiler cells over the given translation units. Returns the
// final overall pass status.
bool runMatrix(const Config &Cfg, const std::vector<CompileEntry> &GccEntries,
               const std::vector<CompileEntry> &ClangEntries,
               const std::string &Gcc, const std::string &Clang,
               std::vector<CellResult> &Cells);

// Verify the normalized .c sets from both databases match.
bool verifyTuSetsMatch(const std::vector<CompileEntry> &Gcc,
                       const std::vector<CompileEntry> &Clang,
                       std::string &Error);

} // namespace green

#endif // GREEN_DRIVER_MATRIX_H
