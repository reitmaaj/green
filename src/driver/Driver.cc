#include "Driver.h"

#include "Matrix.h"
#include "Process.h"

namespace green
{

bool loadDatabases(const Config &Cfg, std::vector<CompileEntry> &Gcc,
                   std::vector<CompileEntry> &Clang, std::string &Error)
{
    std::string E1, E2;
    if (!loadCompileDB(Cfg.GccCompileDB, Gcc, E1))
    {
        Error = E1;
        return false;
    }
    if (!loadCompileDB(Cfg.ClangCompileDB, Clang, E2))
    {
        Error = E2;
        return false;
    }
    if (!verifyTuSetsMatch(Gcc, Clang, Error))
        return false;
    return true;
}

bool resolveTools(std::string &Gcc, std::string &Clang, std::string &Tidy,
                  std::string &Format)
{
    Gcc = whichTool("gcc");
    Clang = whichTool("clang");
    Tidy = whichTool("clang-tidy");
    Format = whichTool("clang-format");
    return !Gcc.empty() && !Clang.empty() && !Tidy.empty() && !Format.empty();
}

} // namespace green
