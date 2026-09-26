#include "CompileDB.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/JSON.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace green
{

// -- command-line tokenization ----------------------------------------------

static std::vector<std::string> tokenizeCommand(const std::string &Cmd)
{
    std::vector<std::string> Out;
    size_t i = 0;
    while (i < Cmd.size())
    {
        while (i < Cmd.size() && std::isspace((unsigned char)Cmd[i]))
            ++i;
        if (i >= Cmd.size())
            break;
        std::string Tok;
        char Quote = 0;
        while (i < Cmd.size())
        {
            char C = Cmd[i];
            if (Quote)
            {
                if (C == Quote)
                {
                    Quote = 0;
                    ++i;
                }
                else if (C == '\\' && i + 1 < Cmd.size())
                {
                    Tok += Cmd[i + 1];
                    i += 2;
                }
                else
                {
                    Tok += C;
                    ++i;
                }
            }
            else
            {
                if (C == '"' || C == '\'')
                {
                    Quote = C;
                    ++i;
                }
                else if (std::isspace((unsigned char)C))
                {
                    break;
                }
                else
                {
                    Tok += C;
                    ++i;
                }
            }
        }
        if (!Tok.empty())
            Out.push_back(std::move(Tok));
    }
    return Out;
}

// -- normalization -----------------------------------------------------------

static bool keepArg(const std::string &A)
{
    // drop the compiler driver name (first token)
    return true;
}

static bool stripArg(const std::string &A, bool &SkipNext)
{
    if (SkipNext)
    {
        SkipNext = false;
        return true;
    }
    if (A == "-std=c89" || A == "-std=c90" || A == "-std=c99" ||
        A == "-std=c11" || A == "-std=c17" || A == "-std=c23" ||
        A == "-std=iso9899:1990" || A == "-std=gnu89" || A == "-std=gnu99" ||
        A == "-std=gnu11" || A == "-std=gnu17" || A == "-std=gnu23")
        return true;
    if (A == "-c" || A == "-fsyntax-only" || A == "-E")
        return true;
    if (A == "-o")
    {
        SkipNext = true;
        return true;
    }
    if (llvm::StringRef(A).starts_with("-W"))
        return true;
    if (A == "-MD" || A == "-MMD" || A == "-MP" || A == "-MG" || A == "-MF" ||
        A == "-MT" || A == "-MQ")
        return true;
    if (A == "-MF" || A == "-MT" || A == "-MQ")
    {
        SkipNext = true;
        return true;
    }
    if (A == "-g" || A == "-O0" || A == "-O1" || A == "-O2" || A == "-O3" ||
        A == "-Os" || A == "-Ofast")
        return true;
    return false;
}

static void normalizeArgs(const std::vector<std::string> &In,
                          std::vector<std::string> &Out)
{
    bool SkipNext = false;
    for (size_t i = 0; i < In.size(); ++i)
    {
        const std::string &A = In[i];
        if (i == 0)
            continue; // compiler driver
        if (A == "-fsyntax-only")
            continue;
        if (stripArg(A, SkipNext))
            continue;
        Out.push_back(A);
    }
}

// -- API ---------------------------------------------------------------------

std::vector<std::string> greenBaseline(const std::string &Std)
{
    std::string Opts = "-std=" + Std;
    return {Opts,
            "-pedantic-errors",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Wconversion",
            "-Wsign-conversion",
            "-Wstrict-prototypes",
            "-Wmissing-prototypes",
            "-Wold-style-definition",
            "-Wundef",
            "-Wshadow",
            "-Wformat=2",
            "-Wcast-qual",
            "-Wno-long-long",
            "-fsyntax-only"};
}

std::vector<std::string> commandForCell(const CompileEntry &E,
                                        const std::string &Compiler,
                                        const std::string &Std)
{
    std::vector<std::string> Args;
    Args.push_back(Compiler);
    for (const auto &A : E.Args)
        Args.push_back(A);
    auto Baseline = greenBaseline(Std);
    for (const auto &A : Baseline)
        Args.push_back(A);
    Args.push_back(E.File);
    return Args;
}

bool loadCompileDB(const std::string &Path, std::vector<CompileEntry> &Out,
                   std::string &Error)
{
    std::ifstream In(Path);
    if (!In)
    {
        Error = "cannot open compile database: " + Path;
        return false;
    }
    std::ostringstream Buf;
    Buf << In.rdbuf();

    llvm::Expected<llvm::json::Value> Parsed = llvm::json::parse(Buf.str());
    if (!Parsed || !Parsed->getAsArray())
    {
        Error = "invalid compile database JSON: " + Path;
        return false;
    }
    Out.clear();
    for (const auto &V : *Parsed->getAsArray())
    {
        const llvm::json::Object *Obj = V.getAsObject();
        if (!Obj)
            continue;
        const llvm::json::Value *F = Obj->get("file");
        const llvm::json::Value *D = Obj->get("directory");
        const llvm::json::Value *Cmd = Obj->get("command");
        if (!F || !F->getAsString())
            continue;
        std::string File = std::string(*F->getAsString());
        if (!llvm::StringRef(File).ends_with(".c"))
            continue;
        CompileEntry E;
        E.File = File;
        if (D && D->getAsString())
            E.Directory = std::string(*D->getAsString());
        std::vector<std::string> Raw;
        if (Cmd && Cmd->getAsString())
            Raw = tokenizeCommand(std::string(*Cmd->getAsString()));
        else if (const llvm::json::Value *Args = Obj->get("arguments"))
        {
            if (const llvm::json::Array *A = Args->getAsArray())
                for (const auto &Elem : *A)
                    if (Elem.getAsString())
                        Raw.push_back(std::string(*Elem.getAsString()));
        }
        normalizeArgs(Raw, E.Args);
        Out.push_back(std::move(E));
    }
    if (Out.empty())
    {
        Error = "compile database contains no .c translation units: " + Path;
        return false;
    }
    return true;
}

} // namespace green
