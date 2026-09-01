#include "Fix.h"

#include "Driver.h"
#include "Process.h"

namespace green
{

int runFix(const std::string &Format, const std::string &Tidy,
           const std::string &Plugin, const std::string &Profile,
           const std::string &CompileDBDir, const std::string &TidyConfig,
           const std::vector<std::string> &Files)
{
    int Status = EXIT_GREEN;
    // 1. Format first so braces/alignment are canonical before applying fixes.
    auto F = runCommand({Format, "-style=file:" + Profile, "-i"});
    (void)F;

    // 2. Apply safe clang-tidy fix-its only.
    const std::string Checks =
        "-checks=-*,readability-braces-around-statements,green-null,"
        "green-transition-boundary";
    for (const auto &File : Files)
    {
        std::vector<std::string> Args = {
            Tidy, "-load=" + Plugin, Checks,  TidyConfig,
            "-p", CompileDBDir,      "--fix", File};
        auto R = runCommand(Args);
        if (R.ExitCode != 0)
            Status = EXIT_VIOLATION;
    }
    return Status;
}

} // namespace green
