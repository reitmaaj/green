#include "Matrix.h"

#include "Process.h"

#include <algorithm>
#include <set>

namespace green
{

bool verifyTuSetsMatch(const std::vector<CompileEntry> &Gcc,
                       const std::vector<CompileEntry> &Clang,
                       std::string &Error)
{
    std::set<std::string> G, C;
    for (const auto &E : Gcc)
        G.insert(E.File);
    for (const auto &E : Clang)
        C.insert(E.File);
    if (G == C)
        return true;
    std::set<std::string> OnlyG, OnlyC;
    for (const auto &F : G)
        if (!C.count(F))
            OnlyG.insert(F);
    for (const auto &F : C)
        if (!G.count(F))
            OnlyC.insert(F);
    std::string Msg = "GCC and Clang translation-unit sets do not match";
    if (!OnlyG.empty())
    {
        Msg += "; only in gcc:";
        for (const auto &F : OnlyG)
            Msg += " " + F;
    }
    if (!OnlyC.empty())
    {
        Msg += "; only in clang:";
        for (const auto &F : OnlyC)
            Msg += " " + F;
    }
    Error = Msg;
    return false;
}

static bool runCell(const std::string &Compiler,
                    const std::vector<CompileEntry> &Entries,
                    const std::string &Std, CellResult &Cell,
                    bool &ToolUnavailable)
{
    std::string Combined;
    int Code = 0;
    for (const auto &E : Entries)
    {
        std::string CompilerName = Compiler;
        auto Cmd = commandForCell(E, Compiler, Std);
        auto R = runCommand(Cmd, E.Directory);
        if (R.ExitCode != 0)
        {
            // A non-0 here might be a capability/option error (exit 3-like)
            // or a source violation (exit 1). We surface stderr.
            Combined += R.Stderr;
            Code = 1;
        }
    }
    if (Code == 0)
    {
        Cell.Pass = true;
        return true;
    }
    // Distinguish source violations from unsupported options: re-run and, if
    // the compiler complains about an unknown flag, mark toolchain unavailable.
    if (Combined.find("unrecognized command-line option") !=
            std::string::npos ||
        Combined.find("unrecognized command line option") !=
            std::string::npos ||
        Combined.find("invalid option") != std::string::npos)
    {
        ToolUnavailable = true;
    }
    Cell.Output = Combined;
    return false;
}

bool runMatrix(const Config &Cfg, const std::vector<CompileEntry> &GccEntries,
               const std::vector<CompileEntry> &ClangEntries,
               const std::string &Gcc, const std::string &Clang,
               std::vector<CellResult> &Cells)
{
    bool GccUnavailable = false, ClangUnavailable = false;
    CellResult GccC89{"GCC C89"};
    CellResult GccC23{"GCC C23"};
    CellResult ClangC89{"Clang C89"};
    CellResult ClangC23{"Clang C23"};
    runCell(Gcc, GccEntries, "c89", GccC89, GccUnavailable);
    runCell(Gcc, GccEntries, "c23", GccC23, GccUnavailable);
    runCell(Clang, ClangEntries, "c89", ClangC89, ClangUnavailable);
    runCell(Clang, ClangEntries, "c23", ClangC23, ClangUnavailable);
    Cells = {GccC89, GccC23, ClangC89, ClangC23};
    return GccC89.Pass && GccC23.Pass && ClangC89.Pass && ClangC23.Pass;
}

} // namespace green
