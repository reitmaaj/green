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

// True when Key is one of the top-level block-list keys.
static bool isBlockListKey(llvm::StringRef Key)
{
    return Key == "project_roots" || Key == "exclude" ||
           Key == "compatibility_paths" || Key == "pure_functions";
}

// Split an inline `[a, b]` list (or a bare scalar) into trimmed elements.
// An empty or `[]` value yields nothing.
static void parseInlineList(const std::string &Value,
                            std::vector<std::string> &Out)
{
    llvm::StringRef V = Value;
    V = V.trim();
    if (V.empty())
        return;
    if (V.front() == '[' && V.back() == ']')
        V = V.slice(1, V.size() - 1);
    else if (V.front() == '[' || V.back() == ']')
        return; // malformed bracket; leave empty rather than invent entries
    llvm::SmallVector<llvm::StringRef, 8> Parts;
    V.split(Parts, ',');
    for (llvm::StringRef P : Parts)
    {
        P = P.trim();
        if (!P.empty())
            Out.push_back(P.str());
    }
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

    // Non-null only while the parser is collecting the `- item` lines beneath
    // an active block-list header. compile_commands is a mapping, not a list.
    std::vector<std::string> *ListTarget = nullptr;
    bool InCompileCommands = false;
    std::string Line;
    bool HaveCompileCommands = false;
    bool HaveVersion = false;

    auto fail = [&](const std::string &Msg)
    {
        Error = Msg;
        return false;
    };

    while (std::getline(Lines, Line))
    {
        std::string T = trim(Line);
        if (T.empty() || T[0] == '#')
            continue;
        size_t Lead = Line.find_first_not_of(' ');
        bool Indented = Lead != std::string::npos && Lead > 0;
        // block-list item
        if (Lead != std::string::npos && Line[Lead] == '-')
        {
            if (!ListTarget)
                return fail("orphaned list item: " + T);
            ListTarget->push_back(trim(Line.substr(Line.find('-') + 1)));
            continue;
        }
        size_t Colon = Line.find(':');
        if (Colon == std::string::npos)
            continue;
        std::string Key = trim(Line.substr(0, Colon));
        std::string Value = trim(Line.substr(Colon + 1));

        if (!Indented)
        {
            // Top-level declarations dispatch explicitly. An empty-value
            // header must NOT reset the parser and swallow the list it opens.
            if (Key == "compile_commands")
            {
                InCompileCommands = true;
                ListTarget = nullptr;
                HaveCompileCommands = true;
                continue;
            }
            if (isBlockListKey(Key))
            {
                InCompileCommands = false;
                if (Value.empty())
                {
                    // block-list header: collect following `- item` lines
                    if (Key == "project_roots")
                        ListTarget = &Out.ProjectRoots;
                    else if (Key == "exclude")
                        ListTarget = &Out.Exclude;
                    else if (Key == "compatibility_paths")
                        ListTarget = &Out.CompatibilityPaths;
                    else
                        ListTarget = &Out.PureFunctions;
                }
                else
                {
                    // inline list such as `pure_functions: []`
                    ListTarget = nullptr;
                    if (Key == "project_roots")
                        parseInlineList(Value, Out.ProjectRoots);
                    else if (Key == "exclude")
                        parseInlineList(Value, Out.Exclude);
                    else if (Key == "compatibility_paths")
                        parseInlineList(Value, Out.CompatibilityPaths);
                    else
                        parseInlineList(Value, Out.PureFunctions);
                }
                continue;
            }
            if (Key == "version")
            {
                if (Value.empty())
                    return fail("empty 'version' value");
                Out.Version = std::stoi(Value);
                HaveVersion = true;
                continue;
            }
            return fail("unknown configuration key: " + Key);
        }
        // Indented sub-keys are valid only beneath compile_commands.
        if (InCompileCommands && Key == "gcc")
        {
            Out.GccCompileDB = Value;
            continue;
        }
        if (InCompileCommands && Key == "clang")
        {
            Out.ClangCompileDB = Value;
            continue;
        }
        return fail("unknown configuration key: " + Key);
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
