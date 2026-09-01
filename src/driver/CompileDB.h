#ifndef GREEN_DRIVER_COMPILEDB_H
#define GREEN_DRIVER_COMPILEDB_H

#include <string>
#include <vector>

namespace green
{

struct CompileEntry
{
    std::string File;              // path to the .c translation unit
    std::string Directory;         // original working directory
    std::vector<std::string> Args; // normalized green command-line args
};

// Load a compile_commands.json, normalize each command per the green profile,
// and return the entries plus the set of normalized .c files.
bool loadCompileDB(const std::string &Path, std::vector<CompileEntry> &Out,
                   std::string &Error);

// The canonical green compiler baseline, appended after normalized args.
std::vector<std::string> greenBaseline(const std::string &Std);

// Produce a normalized per-TU command for a given standard ("c89"/"c23").
std::vector<std::string> commandForCell(const CompileEntry &E,
                                        const std::string &Compiler,
                                        const std::string &Std);

} // namespace green

#endif // GREEN_DRIVER_COMPILEDB_H
