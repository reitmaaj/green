#include "Outline.h"

#include "Driver.h"
#include "Process.h"

#include <iostream>

namespace green
{
namespace
{

// Full green semantic/outline/brace check set (mirrors the driver lint set).
const char *kFullChecks =
    "-checks=-*,green-hidden-control,green-transition-boundary,"
    "green-effect-boundary,green-pure-contract,green-cast-boundary,"
    "green-null,green-declaration,green-fallthrough,green-preprocessor,"
    "green-toolchain-branching,green-outline,"
    "readability-braces-around-statements";

// Only the outline gate: does the file still contain un-extracted basic
// blocks? Uses C89; the gate is standard-independent.
bool fileNeedsOutline(const std::string &Tidy, const std::string &Plugin,
                      const std::string &CompileDBDir,
                      const std::string &TidyConfig, const std::string &File)
{
    CommandResult R = runCommand({Tidy, "-load=" + Plugin,
                                  "-checks=-*,green-outline", TidyConfig, "-p",
                                  CompileDBDir, "--extra-arg=-std=c89", File});
    return R.Stdout.find("[green-outline]") != std::string::npos ||
           R.Stderr.find("[green-outline]") != std::string::npos;
}

// Verify a transformed file is green-clean under both standards.
int verifyClean(const std::string &Tidy, const std::string &Plugin,
                const std::string &CompileDBDir, const std::string &TidyConfig,
                const std::string &File)
{
    int Status = EXIT_GREEN;
    for (const char *Std : {"c89", "c23"})
    {
        CommandResult R = runCommand(
            {Tidy, "-load=" + Plugin, kFullChecks, TidyConfig, "-p",
             CompileDBDir, "--extra-arg=-std=" + std::string(Std), File});
        if (!R.Stdout.empty())
            std::cout << R.Stdout;
        if (!R.Stderr.empty())
            std::cout << R.Stderr;
        if (R.ExitCode == 1)
            Status = EXIT_VIOLATION;
        else if (R.ExitCode != 0)
            Status = EXIT_UNAVAILABLE;
    }
    return Status;
}

} // namespace

int runOutline(const std::string &Tidy, const std::string &Plugin,
               const std::string &OutlineBin, const std::string &Format,
               const std::string &Profile, const std::string &CompileDBDir,
               const std::string &TidyConfig,
               const std::vector<std::string> &Files)
{
    int Status = EXIT_GREEN;
    for (const auto &File : Files)
    {
        if (!fileNeedsOutline(Tidy, Plugin, CompileDBDir, TidyConfig, File))
        {
            std::cout << "outline: " << File << " already outlined\n";
            continue;
        }
        std::cout << "outline: " << File << "\n";
        CommandResult T = runCommand({OutlineBin, "-p", CompileDBDir, File});
        if (!T.Stdout.empty())
            std::cout << T.Stdout;
        if (!T.Stderr.empty())
            std::cerr << T.Stderr;
        if (T.ExitCode != 0)
        {
            std::cerr << "outline: transform failed for " << File << "\n";
            Status = EXIT_VIOLATION;
            continue;
        }
        CommandResult F =
            runCommand({Format, "-style=file:" + Profile, "-i", File});
        if (F.ExitCode != 0)
            Status = EXIT_VIOLATION;
        int Clean = verifyClean(Tidy, Plugin, CompileDBDir, TidyConfig, File);
        if (Clean != EXIT_GREEN)
        {
            std::cerr << "outline: output of " << File
                      << " is not green-clean\n";
            Status = EXIT_VIOLATION;
        }
    }
    return Status;
}

} // namespace green
