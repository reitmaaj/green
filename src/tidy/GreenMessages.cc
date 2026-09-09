// green-tidy plugin: single-line, LLM-consumable diagnostic messages.
//
// Every builder returns one self-contained line (no newlines) shaped as
//     <terse summary>; WHY <principle>; CONTEXT <dynamic facts>; FIX <canonical
//     remedy>
// and is pure: dynamic facts are passed in as plain strings by the checks.
// The file must stay free of Clang/LLVM includes so the builders can be unit
// tested in a standalone binary. Every FIX example itself satisfies the
// profile (braces, prefix ++/--, NULL, single-object declarations, (void)
// prototypes, no hidden-control operators).

#include "GreenMessages.h"

namespace clang
{
namespace tidy
{
namespace msg
{

// Collapse every run of whitespace (including newlines and tabs) to a single
// space and trim, so source snippets keep the message on one line.
std::string cleanSnippet(const std::string &Text)
{
    std::string Out;
    bool PendingSpace = false;
    for (char C : Text)
    {
        if (C == ' ' || C == '\t' || C == '\n' || C == '\r')
        {
            PendingSpace = !Out.empty();
            continue;
        }
        if (PendingSpace)
        {
            Out += ' ';
            PendingSpace = false;
        }
        Out += C;
    }
    return Out;
}

std::string hiddenControlOperator(const std::string &Sym)
{
    std::string Msg = "hidden control operator '" + Sym + "'";
    if (Sym == "&&" || Sym == "||")
    {
        Msg +=
            "; WHY: '&&' and '||' short-circuit: the right operand runs only "
            "when the left operand leaves the outcome undecided, so they "
            "smuggle value-dependent control flow into an expression; green "
            "requires control flow to appear as explicit statement structure";
        if (Sym == "&&")
        {
            Msg += "; CONTEXT: a boolean value ('if (a && b) { }', "
                   "'while (x && y) { }', or 'r = a && b;') formed by "
                   "short-circuit conjunction";
            Msg += "; FIX: nest explicit braced if statements so each operand "
                   "becomes its own decision: 'if (a && b) { BODY }' becomes "
                   "'if (a) { if (b) { BODY } }' (nested ifs); if the operands "
                   "are plain booleans with no short-circuit semantics needed, "
                   "compute them into locals first and test the locals";
        }
        else
        {
            Msg += "; CONTEXT: a boolean value ('if (a || b) { }', "
                   "'while (x || y) { }', or 'r = a || b;') formed by "
                   "short-circuit disjunction";
            Msg +=
                "; FIX: rewrite the disjunction as explicit braced branches: "
                "'if (a || b) { BODY }' becomes 'if (a) { BODY } else if "
                "(b) { BODY }' (BODY may be a single call to a shared "
                "worker), or test the negated first operand: "
                "'if (!a) { } else { BODY }' when only b can run the body";
        }
    }
    else
    {
        Msg += "; WHY: the comma operator evaluates its left operand, discards "
               "the value, and only then evaluates the right operand, so it "
               "sequences effects inside an expression and hides their order";
        Msg += "; CONTEXT: ',' is used as an operator here, not as the "
               "separator between function arguments, declarators, initializer "
               "elements, or for clauses (those separators are accepted and "
               "must not be changed)";
        Msg += "; FIX: split the comma expression into separate statements so "
               "each operand is its own explicit transition, e.g. "
               "'x = f(), g();' becomes 'f();' then 'x = g();'";
    }
    return Msg;
}

std::string hiddenControlTernary()
{
    return "hidden control operator '?:'"
           "; WHY: the conditional operator is value-dependent control flow: "
           "exactly one branch evaluates and its value is selected, hiding a "
           "decision inside an expression; green requires decisions to be "
           "explicit statement structure"
           "; CONTEXT: '?:' selects a value in expression position here"
           "; FIX: replace the conditional with an if/else that assigns the "
           "result: 'x = c ? a : b;' becomes "
           "'if (c) { x = a; } else { x = b; }'"
           "; if only one branch computes, move that computation into a worker "
           "function called from the branch";
}

std::string transitionAssignment()
{
    return "assignment must form a complete transition"
           "; WHY: a mutation is a state transition and must be visible as "
           "such; an assignment embedded inside a larger expression hides the "
           "state change and its evaluation order among the surrounding value "
           "computation"
           "; CONTEXT: this assignment is embedded in a larger expression "
           "instead of standing alone"
           "; FIX: make the assignment the whole standalone statement "
           "('x = y;', 'x += n;'), or, when it drives a loop, the sole "
           "transition of one for clause ('for (i = 0; i < n; ++i)'); "
           "a single for clause must not carry more than one transition; "
           "split nested assignments such as 'x = (y = z);' into separate "
           "statements: 'y = z;' then 'x = y;'";
}

std::string transitionPostfix(const std::string &Op)
{
    return "postfix update is not accepted; use the prefix form"
           "; WHY: the postfix form 'i" +
           Op +
           "' yields the old value and defers the update, so a value and a "
           "mutation share one expression; green accepts only the prefix form, "
           "where the update is the entire transition and no old value leaks "
           "into surrounding computation"
           "; CONTEXT: postfix '" +
           Op +
           "' updates storage while its result is consumed by the enclosing "
           "expression"
           "; FIX: use the prefix form '" +
           Op +
           "i' as a standalone statement or for clause; if the code needed the "
           "old value, restructure so the update never shares an expression "
           "with its consumer";
}

std::string transitionEmbeddedUpdate()
{
    return "update must form a complete transition"
           "; WHY: '++'/'--' mutate state and must be visible as complete "
           "transitions; an update embedded inside a larger expression hides "
           "the mutation among surrounding value computation"
           "; CONTEXT: a prefix update is embedded in a larger expression "
           "instead of standing alone"
           "; FIX: give the update its own standalone statement or for clause: "
           "'++i;', '--n;', 'for (i = 0; i < n; ++i)'; only the prefix form "
           "is accepted, and a single for clause must not carry more than one "
           "transition";
}

std::string effectCall(bool Indirect, const std::string &Callee,
                       const std::string &Placement)
{
    std::string Context =
        Indirect ? "an indirect call through a function pointer (never "
                   "provably pure)"
                 : "the call to '" + Callee + "'";
    std::string PlacementPhrase = Placement.empty()
                                      ? "inside a larger expression"
                                      : "consumed as " + Placement;
    return "effectful call must form a complete transition"
           "; WHY: an effectful call has side effects; green requires every "
           "effect to form a complete transition so the effect and its result "
           "are explicit, and forbids burying an effect inside a value "
           "expression"
           "; CONTEXT: " +
           Context + " is " + PlacementPhrase +
           " rather than standing alone"
           "; FIX: give the effect one of the accepted shapes: a standalone "
           "discarded statement ('f();' or '(void)f();'), the sole right-hand "
           "side of an assignment as a result binding ('r = f();', with the "
           "assignment itself a complete statement), or a for init/increment "
           "clause ('for (...; ...; step())'); a call that is genuinely pure "
           "may be used in expression position only after it is proven pure: "
           "define it directly below the GREEN_PURE marker or list its name "
           "under pure_functions in green.yaml; indirect calls are never "
           "proven pure and must always form a complete transition";
}

std::string pureContract(const std::string &Reason, const std::string &Function,
                         const std::string &Evidence)
{
    std::string Context = "the body of '" + Function + "'";
    if (!Evidence.empty())
        Context += " contains " + Evidence;
    return "false PURE annotation: body contains " + Reason +
           "; WHY: GREEN_PURE promises that the function performs no visible "
           "state mutation, no effectful call, no volatile access, and no "
           "other unproven call; callers in expression position rely on that "
           "promise, so an unearned annotation is unsound"
           "; CONTEXT: " +
           Context + "; FIX: if the body truly performs " + Reason +
           ", the function is not pure: remove the GREEN_PURE marker (and the "
           "name from pure_functions in green.yaml, if listed) or restructure "
           "so the offending operation leaves the function; when a marker "
           "stays, it must sit on its own token line immediately before the "
           "function definition";
}

std::string castRedundant(const std::string &Type)
{
    return "cast from a type to its same canonical type is redundant"
           "; WHY: casting a value to its own canonical type crosses no "
           "semantic boundary, so the cast adds noise without marking an "
           "intent; green accepts casts only when they mark a real domain or "
           "width boundary"
           "; CONTEXT: the cast produces '" +
           Type +
           "', the same canonical type as its operand"
           "; FIX: remove the cast; the operand already has the required "
           "type";
}

std::string castDiscardsQualifiers()
{
    return "cast discards const/volatile qualification"
           "; WHY: casting away const (or volatile) lets code modify data that "
           "was declared read-only or access it outside the volatile contract; "
           "modifying an object that is truly const through such a cast is "
           "undefined behavior"
           "; CONTEXT: the cast's target type drops a qualification that the "
           "source type carries"
           "; FIX: do not cast the qualification away; redesign the "
           "interface so const and volatile propagate (take or return the "
           "qualified type), or make the object non-const at its declaration "
           "if mutation is genuinely intended";
}

std::string castIntPointer(const std::string &Src, const std::string &Dst)
{
    return "representation escape: integer/pointer cast"
           "; WHY: the representations of integers and pointers are not "
           "interchangeable; converting between them is implementation-defined "
           "or undefined and is exactly the kind of hidden platform coupling "
           "green exists to surface"
           "; CONTEXT: a cast from '" +
           Src + "' to '" + Dst +
           "' crosses the integer/pointer boundary"
           "; FIX: stay in one domain: keep the value as an object pointer "
           "(store data through 'void *' or the concrete pointer type, "
           "converting between object-pointer types instead), or keep it as "
           "an integer and never re-derive a pointer from it; a null pointer "
           "constant must be spelled NULL, never as an integer cast";
}

std::string castFnPointer(const std::string &Src, const std::string &Dst)
{
    return "representation escape: object/function pointer cast"
           "; WHY: the C standard does not guarantee that object pointers and "
           "function pointers share a representation, and calling a function "
           "through a converted pointer is undefined behavior"
           "; CONTEXT: a cast from '" +
           Src + "' to '" + Dst +
           "' crosses the object/function pointer boundary"
           "; FIX: never convert between the two domains; declare and use the "
           "function pointer type directly ('int (*handler)(int)'), and keep "
           "data in object pointers; a design that needs to store a callable "
           "as data should dispatch through an explicit table or a function "
           "that returns the right function pointer";
}

std::string nullPointer()
{
    return "use NULL for a null pointer constant"
           "; WHY: NULL is the canonical spelling of the null pointer "
           "constant; a bare integer 0 blurs the distinction between a null "
           "pointer and a zero value, and casts like '(void *)0' or "
           "'(struct node *)0' add noise; the only exempt spelling is the "
           "implementation's own NULL expansion"
           "; CONTEXT: this null pointer constant is written without NULL"
           "; FIX: include <stddef.h> and write NULL at this spot "
           "('p = NULL;', 'if (p == NULL) { }', 'return NULL;')";
}

std::string reservedSuffix(const std::string &Name, const std::string &Kind)
{
    std::string Base = Name.size() > 2 ? Name.substr(0, Name.size() - 2) : Name;
    return "type name ends with the reserved '_t' suffix"
           "; WHY: POSIX reserves the '_t' suffix for the implementation's own "
           "types; a type this project defines with that suffix can collide "
           "with a current or future system type once both appear in one "
           "program"
           "; CONTEXT: '" +
           Name + "' is a " + Kind +
           " whose spelling ends in '_t'"
           "; FIX: rename the type so it does not end in '_t', e.g. "
           "rename '" +
           Name + "' to '" + Base +
           "' (capitalized forms like 'Node' are idiomatic for struct tags); "
           "update every use of the type in owned code";
}

std::string multiDeclaration()
{
    return "one declaration must declare exactly one object"
           "; WHY: a multi-declarator declaration shares one type token across "
           "several objects, which hides per-object initialization, storage "
           "class, and intent; green requires declarations to be explicit and "
           "singular"
           "; CONTEXT: a single declaration (local, file scope, or a "
           "struct/union field list) declares more than one object"
           "; FIX: split it into separate declarations, one object each: "
           "'int count, index;' becomes 'int count;' followed by "
           "'int index;'; fields likewise get one declaration per member";
}

std::string prototypeForm()
{
    return "prototype-form functions are mandatory; use (void) for no "
           "parameters"
           "; WHY: an unspecified-parameter list '()' or a K&R definition "
           "suppresses compile-time argument checking, and C89 and C23 treat "
           "'()' differently; green requires a prototype for every function"
           "; CONTEXT: this function is declared or defined without a "
           "prototype ('()' or a K&R parameter list)"
           "; FIX: write a prototype: for no parameters use "
           "'int poll_status(void);', and for parameters declare them in the "
           "parentheses ('int smaller(int a, int b);')";
}

std::string implicitFallthrough()
{
    return "implicit fallthrough; add 'break' or the exact marker "
           "'/* fall through */'"
           "; WHY: a case that flows into the next without an explicit marker "
           "is usually a bug; green requires intentional continuation to be "
           "declared with the exact canonical marker"
           "; CONTEXT: this case body does not terminate and does not carry "
           "the marker"
           "; FIX: either end the case with 'break;' (or 'return ...;' when "
           "the case returns), or, when falling through is intentional, place "
           "the exact text '/* fall through */' as the last statement before "
           "the next case/default; every other spelling ('/* fallthrough */', "
           "'/* FALLTHROUGH */', '// fall through') is rejected";
}

std::string functionLikeMacro(const std::string &Name)
{
    return "function-like macros are forbidden; use a function or an "
           "object-like constant"
           "; WHY: a function-like macro is text substitution with no "
           "prototype checking: arguments can be evaluated more than once or "
           "not at all, operator precedence can silently change meaning, and "
           "the expansion hides real call semantics"
           "; CONTEXT: the project defines the function-like macro '" +
           Name +
           "'"
           "; FIX: replace it with a real function (give it a prototype; a "
           "pure helper may carry the GREEN_PURE marker so calls stay usable "
           "in expressions), or, when the macro only names a fixed value, "
           "with an object-like constant macro";
}

std::string tokenManipulationMacro(const std::string &Name)
{
    return "token-manipulation macros (pasting or stringification) are "
           "forbidden"
           "; WHY: '##' pastes tokens and '#' stringifies them before the "
           "compiler sees the source, performing token surgery that hides the "
           "real structure and defeats every semantic check"
           "; CONTEXT: the project defines the macro '" +
           Name +
           "' using pasting or stringification"
           "; FIX: replace the macro with a function or an object-like "
           "constant; identifier composition and string building belong in "
           "the language, not in the preprocessor";
}

std::string macroHidesControl(const std::string &Name)
{
    return "object-like macro '" + Name +
           "' hides control/transition structure"
           "; WHY: an object-like macro may abstract values and tokens but may "
           "not inject hidden control flow (?:, &&, ||), transitions "
           "(assignment, updates), or statement structure (return, blocks) at "
           "its expansion sites; green reports the innermost owned macro in "
           "the expansion ancestry"
           "; CONTEXT: expanding '" +
           Name +
           "' produces control or transition structure at a use site"
           "; FIX: remove the macro and write the structure explicitly at "
           "each use site, or replace it with a function that performs the "
           "controlled computation; a pure value abstraction (arithmetic, "
           "bitwise, sizeof, void * constants) stays an acceptable "
           "object-like macro";
}

std::string toolchainBranching()
{
    return "compiler/version-conditional preprocessing selects a dialect "
           "rather than inhabiting the intersection"
           "; WHY: green requires one source that is accepted unchanged as "
           "strict C89 and strict C23 under both GCC and Clang; conditionals "
           "on compiler or language-version identity split the source into "
           "dialects instead of living in their intersection"
           "; CONTEXT: this directive tests compiler or version identity "
           "('__GNUC__', '__clang__', '__STDC_VERSION__', and similar macros)"
           "; FIX: remove the conditional and write code that inhabits the "
           "intersection of the four cells; if a narrow, deliberately "
           "platform-specific shim is genuinely required, move it to a file "
           "under compatibility_paths in green.yaml, which is the one "
           "configured exemption (system and third-party headers are exempt "
           "by ownership)";
}

std::string flatInlineWork()
{
    return "inline computation inside a control/block body; extract it into a "
           "worker function"
           "; WHY: green's structural discipline separates controllers from "
           "workers: nested control blocks (if/else branches, loop bodies, "
           "switch case bodies, bare {} blocks) orchestrate with thin "
           "delegation, and worker functions compute; inline computation "
           "inside a nested block hides a testable unit behind control flow"
           "; CONTEXT: a value built from operators or a computed assignment "
           "sits inside a nested block instead of at a function's top level"
           "; FIX: move the computation into a worker function and call it "
           "from the thin block: 'if (cond) { s = s + i; }' becomes a single "
           "result-binding delegation 'if (cond) { s = combine(s, i); }' with "
           "'int combine(int s, int i) { return s + i; }' defined at file "
           "scope (a pure helper may carry GREEN_PURE)";
}

std::string flatGlueRun()
{
    return "more than one inline statement inside a control/block body; "
           "extract "
           "it into a worker function"
           "; WHY: a nested block must be thin: at most one glue statement (a "
           "discarded call, a prefix update, a constant init, or a "
           "call-result binding) per straight-line run; a run of several "
           "statements is orchestration logic that belongs in a named worker"
           "; CONTEXT: a straight-line run inside a nested block carries more "
           "than one glue statement"
           "; FIX: extract the whole run into a worker function and leave the "
           "block with a single delegation: a body holding 'a();' then 'b();' "
           "becomes one call 'run_ab();' to a worker that performs both "
           "steps at its own top level";
}

std::string bracesMissing(const std::string &Kind)
{
    return "controlled body must be wrapped in braces"
           "; WHY: braces delimit exactly what a control statement controls; "
           "an unbraced body silently ends at the first statement, invites "
           "dangling-else mistakes, and makes later edits change meaning"
           "; CONTEXT: this '" +
           Kind +
           "' controls a body that is not a compound statement"
           "; FIX: wrap the body in braces: put '{' on its own line directly "
           "under the control keyword, keep the body indented four spaces, "
           "and close '}' on its own line; keep an 'else if' chain intact "
           "('else if (x) { }' needs no extra nesting)";
}

std::string bracesEmptyBody(const std::string &Kind)
{
    return "empty controlled body must be '{}', not ';'"
           "; WHY: a null statement ';' reads like an accidental empty body "
           "and is easy to mistake for a missing statement; an empty block "
           "'{}' states the empty intent explicitly"
           "; CONTEXT: this '" +
           Kind +
           "' controls an empty body written as ';'"
           "; FIX: replace the ';' with an empty block: "
           "'while (busy()) { }' (braces on their own lines) declares that "
           "the loop intentionally does nothing";
}

} // namespace msg
} // namespace tidy
} // namespace clang
