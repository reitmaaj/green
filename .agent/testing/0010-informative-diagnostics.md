# Testing: informative diagnostics (LLM-consumable findings)

Every green-produced finding must be self-contained and prescriptive. The
messages are single-line (clang-tidy renders each diagnostic as one
`file:line:col: error: <message> [check-name]` line followed by the source
line and caret), so an automated consumer can attribute each sentence to its
finding and act without external rule documentation.

## Message anatomy

SCENARIO every finding explains the rule it enforces
GIVEN a green-* (or green-owned braces) violation
WHEN green reports the diagnostic
THEN the message begins with the terse violation summary
AND contains a WHY section stating the principle the rule enforces
AND contains a FIX section prescribing the accepted code shape(s)

SCENARIO every finding names its offending construct
GIVEN a violation with dynamic facts (operator, function, macro, type name,
      cast types, controlled statement kind)
WHEN green reports the diagnostic
THEN the message carries a CONTEXT section quoting those facts

SCENARIO FIX examples are canonical
GIVEN a message prescribing a rewrite
WHEN the prescription contains example code
THEN the example itself satisfies the profile (braces, prefix ++/--, NULL,
     single-object declarations, `(void)` prototypes, no hidden-control
     operators)

SCENARIO a finding is one line
GIVEN any green diagnostic
WHEN it is printed
THEN its message contains no newline or carriage return
AND the `[check-name]` suffix and the source/caret rendering stay intact

## Violation kinds

SCENARIO hidden control guidance
GIVEN a `&&`/`||`/`,`/`?:` finding
WHEN green reports it
THEN the FIX names the explicit braced rewrite per operator: nested `if`
     chains for `&&`, `else if` chains for `||`, separate statements for `,`,
     and if/else result assignment for `?:`
AND the `,` guidance distinguishes the comma operator from the argument/
     declarator/initializer separator

SCENARIO transition guidance
GIVEN an embedded-assignment, embedded-update, or postfix-update finding
WHEN green reports it
THEN the FIX states that a transition must be a standalone statement or a
     single `for` clause and that only prefix `++`/`--` is accepted

SCENARIO effect-boundary guidance
GIVEN an effectful-call placement finding
WHEN green reports it
THEN the CONTEXT names the callee (or marks an indirect call) and the
     consuming placement
AND the FIX lists the accepted shapes: standalone discarded statement,
     sole-RHS result binding, or a `for` init/inc clause
AND the FIX states the `pure_functions` / `GREEN_PURE` route for a genuinely
     pure function used in expression position

SCENARIO pure-contract guidance
GIVEN a false `GREEN_PURE` annotation finding
WHEN green reports it
THEN the CONTEXT names the function and the offending body statements
AND the FIX says to remove the annotation or make the body truly pure

SCENARIO cast guidance
GIVEN a redundant/qualifier-discarding/representation-escape cast finding
WHEN green reports it
THEN the CONTEXT quotes the source and destination types
AND the FIX says to remove the cast or restructure so no representation
     boundary is crossed

SCENARIO null guidance
GIVEN a non-`NULL` null-pointer-constant finding
WHEN green reports it
THEN the FIX prescribes `NULL` (with `stddef.h` included) for the null
     pointer constant

SCENARIO reserved-suffix guidance
GIVEN an owned type name ending in `_t`
WHEN green reports it
THEN the CONTEXT names the type and its kind (typedef alias or
     struct/union/enum tag)
AND the FIX prescribes a name that does not end in `_t`

SCENARIO declaration guidance
GIVEN a multi-object declaration finding
WHEN green reports it
THEN the FIX prescribes one declaration per object
SCENARIO prototype guidance
GIVEN a K&R or unspecified-parameter function finding
WHEN green reports it
THEN the FIX prescribes a prototype with `(void)` for no parameters

SCENARIO fallthrough guidance
GIVEN an implicit-fallthrough finding
WHEN green reports it
THEN the FIX names the two accepted resolutions: `break;` or the exact
     marker `/* fall through */` as the case's last statement

SCENARIO preprocessor guidance
GIVEN a function-like, token-manipulation, or control-hiding macro finding
WHEN green reports it
THEN the CONTEXT names the macro
AND the FIX prescribes the replacement (function or object-like constant;
     expand-and-inline or explicit statements)

SCENARIO toolchain-branching guidance
GIVEN a compiler/version-conditional directive finding
WHEN green reports it
THEN the FIX states that one source must inhabit the GCC/Clang C89 ∩ C23
     intersection, and names the `compatibility_paths` configuration for
     deliberately narrow shims

SCENARIO braces guidance
GIVEN an unbraced or null-statement controlled body finding
WHEN green reports it
THEN the CONTEXT names the controlled statement kind (`if`/`else`/`while`/
     `for`/`do`)
AND the FIX prescribes wrapping the body in `{}` (empty bodies become `{}`,
     never `;`)

SCENARIO flat guidance
GIVEN an inline-computation or over-glued nested-block finding
WHEN green reports it
THEN the FIX prescribes extracting the computation into a worker function
     called from the thin block

## Driver-level surfaces

SCENARIO matrix cell failures carry diagnostics
GIVEN a failing compiler cell under `green matrix` or `green check`
WHEN green prints the result
THEN it prints the cell identity (compiler and standard), the strict-mode and
     baseline flags the cell enforces, and the compiler's own diagnostics
SCENARIO format failures carry diagnostics
GIVEN a non-canonical file under `green format --check` or `green check`
WHEN green prints the result
THEN it prints a violation line naming the file and the canonical profile
     requirement
AND prints the clang-format diagnostics that localize each non-canonical line

SCENARIO no silent FAILs
GIVEN any FAIL verdict in a driver command
WHEN green exits nonzero
THEN at least one diagnostic line accompanies the verdict
