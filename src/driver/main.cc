#include "CompileDB.h"
#include "Config.h"
#include "Doctor.h"
#include "Driver.h"
#include "Fix.h"
#include "Format.h"
#include "Guide.h"
#include "Matrix.h"
#include "Process.h"
#include "green/version.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Path.h"

#include <algorithm>
#include <iostream>
#include <vector>

using namespace green;

namespace
{

void printUsage()
{
    std::cout
        << "usage: green <command> [options]\n"
           "\n"
           "commands:\n"
           "  check [file...]      full seven-check matrix, lint, and format\n"
           "  format [--check]     verify or rewrite canonical formatting\n"
           "  lint [file...]       clang-tidy semantic checks (C89 and C23)\n"
           "  matrix               four compiler cells only\n"
           "  semantic <file.i>    semantic-only checks on preprocessed input\n"
           "  fix                  apply safe, semantics-preserving fix-its\n"
           "  doctor               validate the toolchain environment\n"
           "  --version            print version and exit\n";
}

std::string requireConfig()
{
    std::string P = "green.yaml";
    return P;
}

// Resolve a relative path against the config base directory.
std::string resolvePath(const std::string &Base, const std::string &Path)
{
    if (Path.empty() || llvm::StringRef(Path).starts_with("/"))
        return Path;
    return Base + "/" + Path;
}

// Join a config list into a single comma-separated value.
std::string joinList(const std::vector<std::string> &V)
{
    std::string Out;
    for (size_t i = 0; i < V.size(); ++i)
    {
        if (i)
            Out += ",";
        Out += V[i];
    }
    return Out;
}

// Build the clang-tidy --config JSON carrying the project ownership options.
// Each green-* check reads its options under the "<check>.<name>" namespace,
// so the options are repeated across every green-* check.
std::string buildTidyConfig(const Config &Cfg)
{
    static const char *Checks[] = {"green-hidden-control",
                                   "green-transition-boundary",
                                   "green-effect-boundary",
                                   "green-pure-contract",
                                   "green-cast-boundary",
                                   "green-null",
                                   "green-reserved-suffix",
                                   "green-declaration",
                                   "green-fallthrough",
                                   "green-preprocessor",
                                   "green-toolchain-branching",
                                   "green-braces",
                                   "green-flat"};
    std::string Roots = joinList(Cfg.ProjectRoots);
    std::string Exclude = joinList(Cfg.Exclude);
    std::string Compat = joinList(Cfg.CompatibilityPaths);
    std::string Pure = joinList(Cfg.PureFunctions);
    std::string Opts;
    for (const char *C : Checks)
    {
        Opts += "{\"key\":\"" + std::string(C) +
                ".ProjectRoots\",\"value\":\"" + Roots + "\"},";
        Opts += "{\"key\":\"" + std::string(C) + ".Exclude\",\"value\":\"" +
                Exclude + "\"},";
        Opts += "{\"key\":\"" + std::string(C) +
                ".CompatibilityPaths\",\"value\":\"" + Compat + "\"},";
        Opts += "{\"key\":\"" + std::string(C) +
                ".PureFunctions\",\"value\":\"" + Pure + "\"},";
    }
    if (!Opts.empty())
        Opts.pop_back();
    return "{\"CheckOptions\":[" + Opts + "]}";
}

int runLint(const std::string &Tidy, const std::string &Plugin,
            const std::string &CompileDBDir, const std::string &TidyConfig,
            const std::vector<std::string> &Files, bool &Fatal)
{
    const std::string Checks =
        "-checks=-*,green-hidden-control,green-transition-boundary,"
        "green-effect-boundary,green-pure-contract,green-cast-boundary,"
        "green-null,green-declaration,green-fallthrough,green-preprocessor,"
        "green-toolchain-branching,green-flat,"
        "green-reserved-suffix,"
        "green-braces";
    int Status = EXIT_GREEN;
    for (const char *Std : {"c89", "c23"})
    {
        for (const auto &File : Files)
        {
            std::vector<std::string> Args = {Tidy,
                                             "-load=" + Plugin,
                                             Checks,
                                             TidyConfig,
                                             "-p",
                                             CompileDBDir,
                                             "--extra-arg=-std=" +
                                                 std::string(Std),
                                             File};
            auto R = runCommand(Args);
            // Distinguish toolchain/plugin problems from source violations.
            if (R.ExitCode == 1)
                Status = EXIT_VIOLATION;
            else if (R.ExitCode != 0)
                Fatal = true;
            if (!R.Stderr.empty())
                std::cout << R.Stderr;
            if (!R.Stdout.empty())
                std::cout << R.Stdout;
            if (R.Stderr.find("error loading plugin") != std::string::npos)
                Fatal = true;
        }
    }
    return Status;
}

} // namespace

int main(int Argc, char **Argv)
{
    std::vector<std::string> Args(Argv + 1, Argv + Argc);
    if (Args.empty())
    {
        printUsage();
        return EXIT_GREEN;
    }
    std::string Cmd = Args[0];
    Args.erase(Args.begin());

    if (Cmd == "--version" || Cmd == "-V")
    {
        std::cout << "green " << GREEN_VERSION << "\n"
                  << "LLVM " << GREEN_LLVM_VERSION << " Clang "
                  << GREEN_CLANG_VERSION << "\n";
        return EXIT_GREEN;
    }
    if (Cmd == "--help" || Cmd == "-h")
    {
        printUsage();
        return EXIT_GREEN;
    }
    if (Cmd == "doctor")
    {
        std::string Gcc, Clang, Tidy, Format;
        resolveTools(Gcc, Clang, Tidy, Format);
        return runDoctor(Gcc, Clang, Tidy, Format, GREEN_PLUGIN_PATH);
    }

    Config Cfg;
    std::string ConfigPath = requireConfig();
    std::string Err;
    if (!parseConfigFile(ConfigPath, Cfg, Err))
    {
        std::cerr << "green: " << Err << "\n";
        return EXIT_CONFIG;
    }
    std::string GccDB = resolvePath(Cfg.BaseDir, Cfg.GccCompileDB);
    std::string ClangDB = resolvePath(Cfg.BaseDir, Cfg.ClangCompileDB);

    // Verify the actual normalized databases used are the resolved ones.
    Cfg.GccCompileDB = GccDB;
    Cfg.ClangCompileDB = ClangDB;

    std::string Gcc, Clang, Tidy, Format;
    resolveTools(Gcc, Clang, Tidy, Format);

    std::vector<CompileEntry> GccEntries, ClangEntries;
    if (!loadDatabases(Cfg, GccEntries, ClangEntries, Err))
    {
        std::cerr << "green: configuration error: " << Err << "\n";
        return EXIT_CONFIG;
    }

    if (Cmd == "matrix")
    {
        std::vector<CellResult> Cells;
        bool Pass = runMatrix(Cfg, GccEntries, ClangEntries, Gcc, Clang, Cells);
        for (const auto &C : Cells)
        {
            std::cout << C.Label << " " << (C.Pass ? "PASS" : "FAIL") << "\n";
            if (!C.Pass)
            {
                std::cout << matrixCellGuide(C.Label) << "\n";
                if (!C.Output.empty())
                {
                    std::cout << C.Output;
                    if (C.Output.back() != '\n')
                        std::cout << "\n";
                }
            }
        }
        return Pass ? EXIT_GREEN : EXIT_VIOLATION;
    }

    // Collect the set of files to process (whole DB if none specified).
    std::vector<std::string> Files;
    if (Args.empty() || Cmd == "format" || Cmd == "fix")
    {
        for (const auto &E : GccEntries)
            Files.push_back(E.File);
    }
    else
    {
        for (const auto &F : Args)
            if (F != "--check")
                Files.push_back(F);
    }
    llvm::SmallString<128> ClangDBDir(ClangDB);
    llvm::sys::path::remove_filename(ClangDBDir);
    std::string CompileDBDir = ClangDBDir.str().str();
    std::string TidyConfig = "--config=" + buildTidyConfig(Cfg);

    if (Cmd == "format")
    {
        bool CheckOnly =
            std::find(Args.begin(), Args.end(), "--check") != Args.end();
        return runFormatCommand(Format, GREEN_FORMAT_PATH, Files, CheckOnly);
    }
    if (Cmd == "fix")
    {
        return runFix(Format, Tidy, GREEN_PLUGIN_PATH, GREEN_FORMAT_PATH,
                      CompileDBDir, TidyConfig, Files);
    }
    if (Cmd == "lint")
    {
        bool Fatal = false;
        int S = runLint(Tidy, GREEN_PLUGIN_PATH, CompileDBDir, TidyConfig,
                        Files, Fatal);
        return Fatal ? EXIT_UNAVAILABLE : S;
    }
    if (Cmd == "semantic")
    {
        if (Args.empty())
        {
            std::cerr << "green: semantic requires a .i file\n";
            return EXIT_CONFIG;
        }
        // Preprocessed input cannot receive normative source-spelling checks.
        bool Fatal = false;
        return runLint(Tidy, GREEN_PLUGIN_PATH, CompileDBDir, TidyConfig, Files,
                       Fatal);
    }
    if (Cmd == "check")
    {
        std::vector<CellResult> Cells;
        bool MatrixPass =
            runMatrix(Cfg, GccEntries, ClangEntries, Gcc, Clang, Cells);
        bool Fatal = false;
        int LintStatus = runLint(Tidy, GREEN_PLUGIN_PATH, CompileDBDir,
                                 TidyConfig, Files, Fatal);
        // Failing cells must not be silent: print guidance and the
        // compiler's own diagnostics above the summary table.
        for (const auto &C : Cells)
        {
            if (C.Pass)
                continue;
            std::cout << matrixCellGuide(C.Label) << "\n";
            if (!C.Output.empty())
            {
                std::cout << C.Output;
                if (C.Output.back() != '\n')
                    std::cout << "\n";
            }
        }
        int FormatStatus =
            runFormatCommand(Format, GREEN_FORMAT_PATH, Files, true);
        std::cout << "            C89    C23\n";
        std::cout << "GCC         " << (Cells[0].Pass ? "PASS" : "FAIL")
                  << "    " << (Cells[1].Pass ? "PASS" : "FAIL") << "\n";
        std::cout << "Clang       " << (Cells[2].Pass ? "PASS" : "FAIL")
                  << "    " << (Cells[3].Pass ? "PASS" : "FAIL") << "\n";
        std::cout << "clang-tidy  "
                  << (LintStatus == EXIT_GREEN ? "PASS" : "FAIL") << "    "
                  << (LintStatus == EXIT_GREEN ? "PASS" : "FAIL") << "\n";
        std::cout << "format      "
                  << (FormatStatus == EXIT_GREEN ? "PASS" : "FAIL") << "\n";
        std::cout << "CGREEN      "
                  << (MatrixPass && LintStatus == EXIT_GREEN &&
                              FormatStatus == EXIT_GREEN
                          ? "PASS"
                          : "FAIL")
                  << "\n";
        if (Fatal)
            return EXIT_UNAVAILABLE;
        if (!MatrixPass || LintStatus != EXIT_GREEN ||
            FormatStatus != EXIT_GREEN)
            return EXIT_VIOLATION;
        return EXIT_GREEN;
    }

    std::cerr << "green: unknown command '" << Cmd << "'\n";
    printUsage();
    return EXIT_CONFIG;
}
