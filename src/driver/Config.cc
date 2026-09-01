#include "Config.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include <fstream>
#include <sstream>

namespace green
{

static std::string trim(llvm::StringRef S)
{
    return S.trim().str();
}

// Parses the fixed green.yaml schema. Unknown keys are errors.
bool parseConfigFile(const std::string &Path, Config &Out, std::string &Error)
{
    std::ifstream In(Path);
    if (!In)
    {
        Error = "cannot open config: " + Path;
        return false;
    }
    std::ostringstream Buf;
    Buf << In.rdbuf();
    std::istringstream Lines(Buf.str());

    llvm::SmallString<128> Cwd;
    llvm::sys::fs::current_path(Cwd);
    Out.BaseDir = Cwd.str().str();

    std::string CurrentSection; // "" | "compile_commands" | list targets
    std::vector<std::string> *ListTarget = nullptr;
    std::string Line;
    bool HaveCompileCommands = false;
    bool HaveVersion = false;

    auto unknown = [&](const std::string &Key)
    {
        Error = "unknown configuration key: " + Key;
        return false;
    };

    while (std::getline(Lines, Line))
    {
        if (trim(Line).empty() || trim(Line)[0] == '#')
            continue;
        // list item
        if (Line.find_first_not_of(' ') != std::string::npos &&
            Line[Line.find_first_not_of(' ')] == '-')
        {
            if (!ListTarget)
                continue;
            ListTarget->push_back(trim(Line.substr(Line.find('-') + 1)));
            continue;
        }
        size_t Colon = Line.find(':');
        if (Colon == std::string::npos)
            continue;
        std::string Key = trim(Line.substr(0, Colon));
        std::string Value = trim(Line.substr(Colon + 1));
        int Indent =
            (int)(Colon - Line.find_first_not_of(' ')) - (int)(Key.size());
        if (Indent < 0)
            Indent = 0;
        bool Indented = Line.find_first_not_of(' ') > 0;

        if (!Indented && Value.empty())
        {
            CurrentSection = Key;
            if (Key == "compile_commands")
                HaveCompileCommands = true;
            ListTarget = nullptr;
            continue;
        }
        if (Key == "version")
        {
            Out.Version = std::stoi(Value);
            HaveVersion = true;
            continue;
        }
        if (Key == "project_roots")
        {
            ListTarget = &Out.ProjectRoots;
            continue;
        }
        if (Key == "exclude")
        {
            ListTarget = &Out.Exclude;
            continue;
        }
        if (Key == "compatibility_paths")
        {
            ListTarget = &Out.CompatibilityPaths;
            continue;
        }
        if (Key == "pure_functions")
        {
            ListTarget = &Out.PureFunctions;
            continue;
        }
        if (CurrentSection == "compile_commands" && Key == "gcc")
        {
            Out.GccCompileDB = Value;
            continue;
        }
        if (CurrentSection == "compile_commands" && Key == "clang")
        {
            Out.ClangCompileDB = Value;
            continue;
        }
        return unknown(Key);
    }

    if (!HaveVersion)
    {
        Error = "missing 'version' key";
        return false;
    }
    if (Out.Version != 1)
    {
        Error = "unsupported configuration version";
        return false;
    }
    if (!HaveCompileCommands || Out.GccCompileDB.empty() ||
        Out.ClangCompileDB.empty())
    {
        Error = "missing compile_commands entries for gcc and clang";
        return false;
    }
    return true;
}

} // namespace green
