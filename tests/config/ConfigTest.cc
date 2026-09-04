// Unit tests for the green.yaml parser.
#include "driver/Config.h"

#include "llvm/ADT/StringRef.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int g_Failures = 0;

void check(bool Cond, const std::string &What)
{
    if (!Cond)
    {
        ++g_Failures;
        std::cerr << "FAIL: " << What << "\n";
    }
}

std::string writeFile(const std::string &Name, const std::string &Body)
{
    std::string Path = Name;
    std::ofstream Out(Path);
    Out << Body;
    return Path;
}

void removeFile(const std::string &Path)
{
    std::remove(Path.c_str());
}

bool has(const std::vector<std::string> &V, llvm::StringRef Want)
{
    for (const std::string &S : V)
        if (S == Want)
            return true;
    return false;
}
} // namespace

int main()
{
    // 1. All block lists populate from top-level headers plus - item lines.
    {
        std::string Path = writeFile("cfgtest_blocklists.yaml", R"(
version: 1

compile_commands:
    gcc: build/gcc/compile_commands.json
    clang: build/clang/compile_commands.json

project_roots:
    - src
    - include

exclude:
    - build/**
    - vendor/**

compatibility_paths:
    - src/compat/**

pure_functions:
    - strlen
    - isdigit
)");
        green::Config Cfg;
        std::string Err;
        bool Ok = green::parseConfigFile(Path, Cfg, Err);
        removeFile(Path);
        check(Ok, "block lists parse (error: " + Err + ")");
        check(Cfg.Version == 1, "version parsed");
        check(Cfg.GccCompileDB == "build/gcc/compile_commands.json",
              "gcc db parsed");
        check(Cfg.ClangCompileDB == "build/clang/compile_commands.json",
              "clang db parsed");
        check(has(Cfg.ProjectRoots, "src") && has(Cfg.ProjectRoots, "include"),
              "project_roots populated from its own header");
        check(has(Cfg.Exclude, "build/**") && has(Cfg.Exclude, "vendor/**"),
              "exclude populated from its own header");
        check(has(Cfg.CompatibilityPaths, "src/compat/**"),
              "compatibility_paths populated");
        check(has(Cfg.PureFunctions, "strlen") &&
                  has(Cfg.PureFunctions, "isdigit"),
              "pure_functions populated from its own header");
    }

    // 2. Inline empty-list spellings used by the shipped default config.
    {
        std::string Path = writeFile("cfgtest_inline.yaml", R"(
version: 1

compile_commands:
    gcc: g.json
    clang: c.json

compatibility_paths: []

pure_functions: []
)");
        green::Config Cfg;
        std::string Err;
        bool Ok = green::parseConfigFile(Path, Cfg, Err);
        removeFile(Path);
        check(Ok, "inline empty lists parse (error: " + Err + ")");
        check(Cfg.PureFunctions.empty(), "inline pure_functions: [] is empty");
        check(Cfg.CompatibilityPaths.empty(),
              "inline compatibility_paths: [] is empty");
    }

    // 3. An orphaned list item is rejected, not silently ignored.
    {
        std::string Path = writeFile("cfgtest_orphan.yaml", R"(
version: 1

compile_commands:
    gcc: g.json
    clang: c.json

- whatever
)");
        green::Config Cfg;
        std::string Err;
        bool Ok = green::parseConfigFile(Path, Cfg, Err);
        removeFile(Path);
        check(!Ok, "orphaned list item is rejected");
        check(Err.find("orphaned list item") != std::string::npos,
              "orphan error mentions the list item");
    }

    // 4. Unknown top-level key is rejected.
    {
        std::string Path = writeFile("cfgtest_unknown.yaml", R"(
version: 1

compile_commands:
    gcc: g.json
    clang: c.json

bogus:
    - x
)");
        green::Config Cfg;
        std::string Err;
        bool Ok = green::parseConfigFile(Path, Cfg, Err);
        removeFile(Path);
        check(!Ok, "unknown top-level key is rejected");
        check(Err.find("bogus") != std::string::npos,
              "unknown-key error names the key");
    }

    // 5. A block list header is not consumed by the gcc/clang mapping.
    {
        std::string Path = writeFile("cfgtest_order.yaml", R"(
version: 1

compile_commands:
    gcc: g.json
    clang: c.json

project_roots:
    - src

exclude:
    - build/**
)");
        green::Config Cfg;
        std::string Err;
        bool Ok = green::parseConfigFile(Path, Cfg, Err);
        removeFile(Path);
        check(Ok, "sequential block headers parse (error: " + Err + ")");
        check(has(Cfg.ProjectRoots, "src"),
              "project_roots survives the exclude header that follows");
        check(has(Cfg.Exclude, "build/**"),
              "exclude items are not dropped by the prior header");
    }

    if (g_Failures != 0)
    {
        std::cerr << "config tests: " << g_Failures << " failure(s)\n";
        return 1;
    }
    std::cout << "config tests: PASS\n";
    return 0;
}
