// Unit tests for the informative-diagnostic message builders.
// Every message must be one line, carry WHY and FIX sections, begin with its
// terse summary, and interpolate the dynamic facts the check passes in.
#include "tidy/GreenMessages.h"
#include "driver/Guide.h"

#include <iostream>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::tidy;

namespace
{
int g_Failures = 0;

void must(bool Cond, const std::string &What)
{
    if (!Cond)
    {
        ++g_Failures;
        std::cerr << "FAIL: " << What << "\n";
    }
}

bool contains(const std::string &Hay, const std::string &Needle)
{
    return Hay.find(Needle) != std::string::npos;
}

void checkMessage(const std::string &Name, const std::string &Msg,
                  const std::string &Summary,
                  const std::vector<std::string> &Wants)
{
    must(contains(Msg, Summary), Name + ": summary prefix '" + Summary + "'");
    must(Msg.find('\n') == std::string::npos,
         Name + ": single-line (no newline)");
    must(Msg.find('\r') == std::string::npos,
         Name + ": single-line (no carriage return)");
    must(contains(Msg, "WHY:"), Name + ": WHY section");
    must(contains(Msg, "FIX:"), Name + ": FIX section");
    for (const std::string &W : Wants)
        must(contains(Msg, W), Name + ": anchor '" + W + "'");
}

void checkGuide(const std::string &Name, const std::string &Msg,
                const std::vector<std::string> &Wants)
{
    must(Msg.find('\n') == std::string::npos,
         Name + ": single-line (no newline)");
    for (const std::string &W : Wants)
        must(contains(Msg, W), Name + ": anchor '" + W + "'");
}
} // namespace

int main()
{
    // -- green-hidden-control ----------------------------------------------
    checkMessage("hidden &&", msg::hiddenControlOperator("&&"),
                 "hidden control operator '&&'",
                 {"short-circuit", "WHY:", "nested", "if (a) { if (b)"});
    checkMessage("hidden ||", msg::hiddenControlOperator("||"),
                 "hidden control operator '||'", {"short-circuit", "else if"});
    checkMessage("hidden comma", msg::hiddenControlOperator(","),
                 "hidden control operator ','",
                 {"separator", "separate statements", "for"});
    checkMessage("hidden ?:", msg::hiddenControlTernary(),
                 "hidden control operator '?:'", {"if", "else"});

    // -- green-transition-boundary ------------------------------------------
    checkMessage("embedded assignment", msg::transitionAssignment(),
                 "assignment must form a complete transition",
                 {"standalone statement", "for clause"});
    {
        std::string M = msg::transitionPostfix("--");
        checkMessage("postfix update", M,
                     "postfix update is not accepted; use the prefix form",
                     {"--i", "prefix"});
    }
    checkMessage("embedded update", msg::transitionEmbeddedUpdate(),
                 "update must form a complete transition",
                 {"standalone statement", "prefix"});

    // -- green-effect-boundary ----------------------------------------------
    checkMessage(
        "effect call",
        msg::effectCall(false, "produce", "an argument of another call"),
        "effectful call must form a complete transition",
        {"produce", "an argument of another call", "result binding",
         "GREEN_PURE", "pure_functions"});
    {
        std::string M = msg::effectCall(true, "", "a condition");
        checkMessage("indirect effect", M,
                     "effectful call must form a complete transition",
                     {"indirect", "a condition"});
    }

    // -- green-pure-contract ------------------------------------------------
    checkMessage("false pure",
                 msg::pureContract("an effectful call", "poll",
                                   "poll_status() at line 12"),
                 "false PURE annotation: body contains an effectful call",
                 {"poll", "GREEN_PURE", "poll_status() at line 12"});

    // -- green-cast-boundary ------------------------------------------------
    checkMessage("redundant cast", msg::castRedundant("unsigned int"),
                 "cast from a type to its same canonical type is redundant",
                 {"unsigned int", "remove"});
    checkMessage("cast discards qualifiers", msg::castDiscardsQualifiers(),
                 "cast discards const/volatile qualification",
                 {"const", "volatile"});
    checkMessage("int/pointer cast",
                 msg::castIntPointer("unsigned long", "void *"),
                 "representation escape: integer/pointer cast",
                 {"unsigned long", "void *", "NULL"});
    checkMessage("object/function pointer cast",
                 msg::castFnPointer("void (*)(int)", "struct node *"),
                 "representation escape: object/function pointer cast",
                 {"void (*)(int)", "struct node *"});

    // -- green-null ---------------------------------------------------------
    checkMessage("null spelling", msg::nullPointer(),
                 "use NULL for a null pointer constant", {"NULL", "stddef.h"});

    // -- green-reserved-suffix ----------------------------------------------
    checkMessage("reserved suffix",
                 msg::reservedSuffix("count_t", "typedef name"),
                 "type name ends with the reserved '_t' suffix",
                 {"count_t", "typedef name", "count"});

    // -- green-declaration --------------------------------------------------
    checkMessage("multi declaration", msg::multiDeclaration(),
                 "one declaration must declare exactly one object",
                 {"one object", "int count, index;"});
    checkMessage("prototype form", msg::prototypeForm(),
                 "prototype-form functions are mandatory; use (void) for no "
                 "parameters",
                 {"(void)", "prototype"});

    // -- green-fallthrough --------------------------------------------------
    checkMessage("implicit fallthrough", msg::implicitFallthrough(),
                 "implicit fallthrough; add 'break' or the exact marker "
                 "'/* fall through */'",
                 {"break", "/* fall through */"});

    // -- green-preprocessor -------------------------------------------------
    checkMessage("function-like macro", msg::functionLikeMacro("MAX"),
                 "function-like macros are forbidden; use a function or an "
                 "object-like constant",
                 {"MAX", "function"});
    checkMessage("token manipulation", msg::tokenManipulationMacro("PASTE"),
                 "token-manipulation macros (pasting or stringification) are "
                 "forbidden",
                 {"PASTE", "##", "#"});
    checkMessage("macro hides control", msg::macroHidesControl("CHOOSE"),
                 "object-like macro 'CHOOSE' hides control/transition "
                 "structure",
                 {"CHOOSE"});

    // -- green-toolchain-branching ------------------------------------------
    checkMessage("toolchain branching", msg::toolchainBranching(),
                 "compiler/version-conditional preprocessing selects a "
                 "dialect rather than inhabiting the intersection",
                 {"intersection", "compatibility_paths"});

    // -- green-flat ---------------------------------------------------------
    checkMessage("flat inline work", msg::flatInlineWork(),
                 "inline computation inside a control/block body; extract it "
                 "into a worker function",
                 {"worker function", "nested block"});
    checkMessage("flat glue run", msg::flatGlueRun(),
                 "more than one inline statement inside a control/block body; "
                 "extract it into a worker function",
                 {"worker function", "thin"});

    // -- green-braces -------------------------------------------------------
    checkMessage("braces missing", msg::bracesMissing("if"),
                 "controlled body must be wrapped in braces", {"if", "{"});
    checkMessage("braces empty body", msg::bracesEmptyBody("while"),
                 "empty controlled body must be '{}', not ';'",
                 {"while", "{}"});

    // -- snippet sanitizer ---------------------------------------------------
    {
        std::string S = msg::cleanSnippet("  a\tb\nc  \n");
        must(S == "a b c", "cleanSnippet collapses whitespace");
        must(S.find('\n') == std::string::npos &&
                 S.find('\t') == std::string::npos,
             "cleanSnippet strips newlines and tabs");
    }

    // -- driver guides -------------------------------------------------------
    checkGuide("matrix cell guide", green::matrixCellGuide("GCC C89"),
               {"GCC C89", "matrix cell FAILED", "-std=c89", "baseline",
                "GCC and Clang", "c23", "C89"});
    checkGuide("format fail guide", green::formatFailGuide("src/demo.c"),
               {"format violation", "src/demo.c", "clang-format profile"});

    if (g_Failures != 0)
    {
        std::cerr << "message tests: " << g_Failures << " failure(s)\n";
        return 1;
    }
    std::cout << "message tests: PASS\n";
    return 0;
}
