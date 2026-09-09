#include "Driver.h"
#include "Guide.h"

#include "Process.h"

#include "llvm/ADT/StringRef.h"

#include <iostream>

namespace green
{

// clang-format with an explicit installed profile is authoritative.
static std::string styleFlag(const std::string &Profile)
{
    return "file:" + Profile;
}

// Print a failing format check's diagnostics behind the guide line so the
// finding is never silent.
static void reportFormatFailure(const std::string &File, const CommandResult &R)
{
    std::cout << formatFailGuide(File) << "\n";
    std::cout << R.Stderr;
    std::cout << R.Stdout;
    if (R.Stderr.empty() && R.Stdout.empty())
        std::cout << "green: (clang-format produced no diagnostics)\n";
}

// Runs clang-format over the given files. If CheckOnly, verifies without
// modifying. Returns 0 on success.
static int runFormat(const std::string &Format, const std::string &Profile,
                     const std::vector<std::string> &Files, bool CheckOnly)
{
    for (const auto &F : Files)
    {
        std::vector<std::string> Args = {Format,
                                         "-style=" + styleFlag(Profile)};
        if (CheckOnly)
            Args.push_back("--dry-run");
        Args.push_back("--Werror");
        Args.push_back(F);
        auto R = runCommand(Args);
        if (R.ExitCode != 0)
        {
            reportFormatFailure(F, R);
            return EXIT_VIOLATION;
        }
        if (!CheckOnly)
            std::cout << R.Stdout; // reformatted source
    }
    return EXIT_GREEN;
}

int runFormatCommand(const std::string &Format, const std::string &Profile,
                     const std::vector<std::string> &Files, bool CheckOnly)
{
    return runFormat(Format, Profile, Files, CheckOnly);
}

} // namespace green
