#include "Process.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"

#include <fstream>
#include <sstream>

using llvm::SmallString;

namespace green
{

std::string whichTool(const std::string &Name)
{
    auto PathOrErr = llvm::sys::findProgramByName(Name);
    if (PathOrErr)
        return *PathOrErr;
    return "";
}

CommandResult runCommand(const std::vector<std::string> &Args,
                         const std::string &Cwd)
{
    std::vector<llvm::StringRef> ArgRefs;
    for (const auto &A : Args)
        ArgRefs.push_back(A);

    SmallString<256> OutPath, ErrPath;
    int OutFD = -1, ErrFD = -1;
    std::error_code EC;
    EC = llvm::sys::fs::createTemporaryFile("green-out", "txt", OutFD, OutPath);
    if (!EC)
        EC = llvm::sys::fs::createTemporaryFile("green-err", "txt", ErrFD,
                                                ErrPath);

    CommandResult Result{127, "", ""};
    if (!EC)
    {
        std::optional<llvm::StringRef> Redirects[3] = {
            std::nullopt, llvm::StringRef(OutPath), llvm::StringRef(ErrPath)};
        std::string ErrMsg;
        bool ExecutionFailed = false;
        std::optional<int> Code = llvm::sys::ExecuteAndWait(
            ArgRefs[0], ArgRefs, std::nullopt, Redirects, 0, 0, &ErrMsg,
            &ExecutionFailed);
        if (Code)
            Result.ExitCode = *Code;
        else if (ExecutionFailed)
            Result.ExitCode = 127;

        std::ifstream OutFile(std::string(OutPath.str()));
        std::ifstream ErrFile(std::string(ErrPath.str()));
        std::ostringstream OutBuf, ErrBuf;
        OutBuf << OutFile.rdbuf();
        ErrBuf << ErrFile.rdbuf();
        Result.Stdout = OutBuf.str();
        Result.Stderr = ErrBuf.str();
    }
    llvm::sys::fs::remove(OutPath);
    llvm::sys::fs::remove(ErrPath);
    return Result;
}

std::string toolVersion(const std::string &Tool)
{
    auto R = runCommand({Tool, "--version"});
    std::istringstream In(R.Stdout);
    std::string Line;
    std::getline(In, Line);
    return Line;
}

} // namespace green
