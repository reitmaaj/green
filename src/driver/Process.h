#ifndef GREEN_DRIVER_PROCESS_H
#define GREEN_DRIVER_PROCESS_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include <string>
#include <vector>

namespace green
{

struct CommandResult
{
    int ExitCode;
    std::string Stdout;
    std::string Stderr;
};

// Locate an executable on PATH; returns empty if not found.
std::string whichTool(const std::string &Name);

// Run a command, capturing stdout/stderr separately.
CommandResult runCommand(const std::vector<std::string> &Args,
                         const std::string &Cwd = "");

// Return the first line of `tool --version`.
std::string toolVersion(const std::string &Tool);

} // namespace green

#endif // GREEN_DRIVER_PROCESS_H
