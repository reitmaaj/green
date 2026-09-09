// green-tidy plugin: semantic and preprocessor checks.
//
// All green-* checks report only project-owned source unless configuration
// explicitly extends their scope. V1 provides no inline NOLINT escape.

#ifndef GREEN_TIDY_GREENCHECKS_H
#define GREEN_TIDY_GREENCHECKS_H

#include "clang-tidy/ClangTidy.h"
#include "clang-tidy/ClangTidyCheck.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/PPCallbacks.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"

#include <string>

namespace clang
{
namespace tidy
{

// Shared helpers ----------------------------------------------------------

// True when the location is inside a configured compatibility path.
bool isCompatibilityPath(SourceLocation Loc, const SourceManager &SM,
                         llvm::StringRef Compatibility);

// Path-based project ownership. Ownership is independent of compiler header
// classification (-isystem, system-header bits, angle vs quoted include).
//
//   if project_roots is nonempty:
//       owned(file) := beneath(canonical(file), any project_root)
//   else:
//       owned(file) := beneath(canonical(file), canonical(dirname(main TU)))
//   owned(file) := owned(file) && !beneath(canonical(file), any exclude)
//
// Containment compares path components, not string prefixes, and paths are
// canonicalized. Source locations that cannot resolve to a real source file
// (virtual/builtin/scratch buffers) are simply not owned.
class Ownership
{
  public:
    Ownership(llvm::StringRef Roots, llvm::StringRef Exclude);
    // Compute canonical roots/excludes and the primary-TU directory once.
    void configure(const SourceManager &SM);
    bool isOwned(SourceLocation Loc, const SourceManager &SM) const;

  private:
    std::string RootsStr;
    std::string ExcludeStr;
    llvm::SmallVector<std::string, 8> Roots;
    llvm::SmallVector<std::string, 8> Exclude;
    std::string MainDir;
    bool Configured = false;
};

// The set of project-owned macros, populated by a per-check PPCallbacks
// during preprocessing. Each check that needs macro-origin attribution keeps
// its own instance; there is no shared global state (clang-tidy may process
// translation units concurrently).
class MacroRegistry
{
  public:
    struct Entry
    {
        std::string Name;
        SourceLocation DefBegin;
        SourceLocation DefEnd;
        bool IsObjectLike;
    };

    void record(llvm::StringRef Name, SourceLocation DefBegin,
                SourceLocation DefEnd, bool IsObjectLike)
    {
        Entries.push_back(Entry{Name.str(), DefBegin, DefEnd, IsObjectLike});
    }
    // Walk the macro-expansion ancestry of Loc and return the innermost
    // project-owned macro responsible, or nullptr. The pointer is valid only
    // until the next modification of the registry.
    const Entry *findGoverning(SourceLocation Loc,
                               const SourceManager &SM) const;

  private:
    llvm::SmallVector<Entry, 8> Entries;
};

// The set of pure function names: the GREEN_PURE marker (macro "GREEN_PURE"
// whose expansion occupies its own token line immediately before a function)
// plus the configured pure_functions list.
class PureRegistry
{
  public:
    void addConfigured(llvm::StringRef List);
    void recordMarkerLine(unsigned Line)
    {
        MarkerLine = Line;
    }
    void clearMarker()
    {
        MarkerLine = 0;
    }
    unsigned markerLine() const
    {
        return MarkerLine;
    }
    bool isPure(llvm::StringRef Name) const
    {
        return Names.count(Name) != 0;
    }
    void addMarked(llvm::StringRef Name)
    {
        Names.insert(Name);
    }

  private:
    llvm::StringSet<> Names;
    unsigned MarkerLine = 0;
};

// green-hidden-control -----------------------------------------------------
class HiddenControlCheck : public ClangTidyCheck
{
  public:
    HiddenControlCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
                             Preprocessor *ModuleExpanderPP) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    Ownership Owned;
    MacroRegistry Macros;
};

// green-transition-boundary -------------------------------------------------
class TransitionBoundaryCheck : public ClangTidyCheck
{
  public:
    TransitionBoundaryCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
                             Preprocessor *ModuleExpanderPP) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    PureRegistry Pure;
    Ownership Owned;
    MacroRegistry Macros;
};

// green-effect-boundary ------------------------------------------------------
// An effectful call (one not proven pure) must form a complete transition: a
// standalone discarded statement, or the sole right-hand side of an
// assignment. Parentheses and implicit or explicit casts are
// placement-transparent wrappers around a call; computing from or consuming
// the wrapped result is still rejected. Cast admissibility is judged solely by
// green-cast-boundary. An indirect call is conservatively effectful.
class EffectBoundaryCheck : public ClangTidyCheck
{
  public:
    EffectBoundaryCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
                             Preprocessor *ModuleExpanderPP) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    PureRegistry Pure;
    Ownership Owned;
};

// green-pure-contract --------------------------------------------------------
class PureContractCheck : public ClangTidyCheck
{
  public:
    PureContractCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context)
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
                             Preprocessor *ModuleExpanderPP) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    PureRegistry Pure;
    bool ReportedPending = false;
};

// green-cast-boundary --------------------------------------------------------
class CastBoundaryCheck : public ClangTidyCheck
{
  public:
    CastBoundaryCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    Ownership Owned;
};

// green-null -----------------------------------------------------------------
class NullCheck : public ClangTidyCheck
{
  public:
    NullCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    Ownership Owned;
};

// green-reserved-suffix -------------------------------------------------------
// The C standard reserves leading underscores for the implementation and POSIX
// reserves the `_t` suffix for the implementation's own types. Reject the `_t`
// suffix on type names the project itself defines (typedef names and
// struct/union/enum tags). Only owned *definitions* are judged; a system type
// that is merely used is declared in a file the project does not own and is
// never reported. Owned files under compatibility_paths are exempt.
class ReservedSuffixCheck : public ClangTidyCheck
{
  public:
    ReservedSuffixCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    Ownership Owned;
    llvm::StringSet<> Reported;
};

// green-declaration ----------------------------------------------------------
class DeclarationCheck : public ClangTidyCheck
{
  public:
    DeclarationCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    Ownership Owned;
};

// green-fallthrough ----------------------------------------------------------
class FallthroughCheck : public ClangTidyCheck
{
  public:
    FallthroughCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    Ownership Owned;
};

// green-preprocessor ---------------------------------------------------------
class PreprocessorCheck : public ClangTidyCheck
{
  public:
    PreprocessorCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
                             Preprocessor *ModuleExpanderPP) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    Ownership Owned;
    MacroRegistry Macros;
};

// green-toolchain-branching --------------------------------------------------
class ToolchainBranchingCheck : public ClangTidyCheck
{
  public:
    ToolchainBranchingCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
                             Preprocessor *ModuleExpanderPP) override;

  private:
    Ownership Owned;
    void checkCondition(SourceRange ConditionRange, const SourceManager &SM);
};

// green-braces ---------------------------------------------------------------
// Mandatory braces (replaces the reused readability-braces-around-statements
// builtin so the message text is green's own): every controlled body
// (if/else/while/do/for) is a compound statement; empty bodies are `{}`,
// never a null statement `;`; an `else if` remains a chain. Fix-its wrap the
// body in braces (or replace `;` with `{}`) for the safe `green fix` flow.
class BracesCheck : public ClangTidyCheck
{
  public:
    BracesCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    void reportBody(const Stmt *Body, const char *Kind, const SourceManager &SM,
                    ASTContext &Ctx);

    Ownership Owned;
};

// green-flat -----------------------------------------------------------------
// Structural decomposition / test-boundary discipline: every nontrivial
// computation reached through control flow should acquire a named function
// boundary so it is independently callable, testable, and analyzable. Inline
// work is welcome at a function's top level (its own straight-line flow). It
// is the smell only when it sits inside a *nested* block - an if/else body, a
// loop body, a switch/case body, or an explicit bare {} block. Every nested
// block must be thin: no inline computation, and at most one glue statement (a
// discarded call, a prefix update, a constant init/assignment, or a
// call-result binding) per straight-line run. Nested control blocks
// orchestrate; worker functions compute.
class FlatCheck : public ClangTidyCheck
{
  public:
    FlatCheck(llvm::StringRef Name, ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context),
          Owned(Options.get("ProjectRoots", ""), Options.get("Exclude", ""))
    {
    }
    void registerMatchers(ast_matchers::MatchFinder *Finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  private:
    Ownership Owned;
};

} // namespace tidy
} // namespace clang

#endif // GREEN_TIDY_GREENCHECKS_H
