#include "Doctor.h"

#include "CompileDB.h"
#include "Driver.h"
#include "Process.h"
#include "green/version.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <iostream>

using llvm::SmallString;

namespace green
{

static bool probeCompilerCapability(const std::string &Compiler,
                                    const std::string &Std)
{
    SmallString<256> Path;
    int FD = -1;
    if (llvm::sys::fs::createTemporaryFile("green-probe", "c", FD, Path))
        return false;
    {
        llvm::raw_fd_ostream Out(FD, /*shouldClose=*/true);
        Out << "int probe_fn(void);\nint probe_fn(void) { return 0; }\n";
    }
    auto Baseline = greenBaseline(Std);
    std::vector<std::string> Args = {Compiler};
    for (const auto &A : Baseline)
        Args.push_back(A);
    Args.push_back(std::string(Path));
    auto R = runCommand(Args);
    llvm::sys::fs::remove(Path);
    return R.ExitCode == 0;
}

int runDoctor(const std::string &Gcc, const std::string &Clang,
              const std::string &Tidy, const std::string &Format,
              const std::string &PluginPath)
{
    std::cout << "green " << GREEN_VERSION << " doctor\n";
    std::cout << "  gcc:          "
              << (Gcc.empty() ? "MISSING" : toolVersion(Gcc)) << "\n";
    std::cout << "  clang:        "
              << (Clang.empty() ? "MISSING" : toolVersion(Clang)) << "\n";
    std::cout << "  clang-tidy:   "
              << (Tidy.empty() ? "MISSING" : toolVersion(Tidy)) << "\n";
    std::cout << "  clang-format: "
              << (Format.empty() ? "MISSING" : toolVersion(Format)) << "\n";
    std::cout << "  plugin LLVM/Clang build: " << GREEN_LLVM_VERSION << " / "
              << GREEN_CLANG_VERSION << "\n";

    bool Fatal = false;
    if (Gcc.empty() || Clang.empty() || Tidy.empty() || Format.empty())
    {
        std::cout << "  FATAL: a required tool is missing\n";
        Fatal = true;
    }
    if (!Gcc.empty())
    {
        bool C89 = probeCompilerCapability(Gcc, "c89");
        bool C23 = probeCompilerCapability(Gcc, "c23");
        std::cout << "  gcc C89 capability: " << (C89 ? "yes" : "no") << "\n";
        std::cout << "  gcc C23 capability: " << (C23 ? "yes" : "no") << "\n";
        if (!C89 || !C23)
            Fatal = true;
    }
    if (Fatal)
        return EXIT_UNAVAILABLE;
    return EXIT_GREEN;
}

} // namespace green
