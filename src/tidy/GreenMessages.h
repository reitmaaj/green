// green-tidy plugin: single-line, LLM-consumable diagnostic messages.
//
// Every builder returns one self-contained line (no newlines) shaped as
//     <terse summary>; WHY <principle>; CONTEXT <dynamic facts>; FIX <canonical
//     remedy>
// and is pure: dynamic facts are passed in as plain strings by the checks.
// The file must stay free of Clang/LLVM includes so the builders can be unit
// tested in a standalone binary.

#ifndef GREEN_TIDY_GREENMESSAGES_H
#define GREEN_TIDY_GREENMESSAGES_H

#include <string>

namespace clang
{
namespace tidy
{
namespace msg
{

// Collapse whitespace and strip newlines so CONTEXT snippets stay single-line.
std::string cleanSnippet(const std::string &Text);

// green-hidden-control
std::string hiddenControlOperator(const std::string &Sym); // && || ,
std::string hiddenControlTernary();

// green-transition-boundary
std::string transitionAssignment();
std::string transitionPostfix(const std::string &Op); // ++ or --
std::string transitionEmbeddedUpdate();

// green-effect-boundary
std::string effectCall(bool Indirect, const std::string &Callee,
                       const std::string &Placement);

// green-pure-contract
std::string pureContract(const std::string &Reason, const std::string &Function,
                         const std::string &Evidence);

// green-cast-boundary
std::string castRedundant(const std::string &Type);
std::string castDiscardsQualifiers();
std::string castIntPointer(const std::string &Src, const std::string &Dst);
std::string castFnPointer(const std::string &Src, const std::string &Dst);

// green-null
std::string nullPointer();

// green-reserved-suffix
std::string reservedSuffix(const std::string &Name, const std::string &Kind);

// green-declaration
std::string multiDeclaration();
std::string prototypeForm();

// green-fallthrough
std::string implicitFallthrough();

// green-preprocessor
std::string functionLikeMacro(const std::string &Name);
std::string tokenManipulationMacro(const std::string &Name);
std::string macroHidesControl(const std::string &Name);

// green-toolchain-branching
std::string toolchainBranching();

// green-flat
std::string flatInlineWork();
std::string flatGlueRun();

// green-braces
std::string bracesMissing(const std::string &Kind); // if/else/while/for/do
std::string bracesEmptyBody(const std::string &Kind);

} // namespace msg
} // namespace tidy
} // namespace clang

#endif // GREEN_TIDY_GREENMESSAGES_H
