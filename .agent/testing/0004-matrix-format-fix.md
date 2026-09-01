# Testing: matrix, format, preprocessed input, fix

SCENARIO C89-only syntax is rejected by the C23 cell
GIVEN source that is only valid in C89
WHEN `green matrix` runs the C23 cells
THEN the C23 cells report failure

SCENARIO C23-only syntax is rejected by the C89 cell
GIVEN source using C23-only syntax (e.g. `nullptr`, `[[fallthrough]]`)
WHEN `green matrix` runs the C89 cells
THEN the C89 cells report failure

SCENARIO new C23 keyword used as a C89 identifier
GIVEN a C23 keyword such as `typeof` used as an identifier
WHEN `green matrix` runs
THEN the C23 cells report failure while C89 cells may pass

SCENARIO mixed declarations and statements
GIVEN a declaration after a statement in a block
WHEN `green matrix` runs the C89 cells
THEN the C89 cells report failure

SCENARIO canonical formatting round-trips
GIVEN source already in canonical form
WHEN `green format --check` runs
THEN exit 0 and no modifications are reported

SCENARIO non-canonical formatting is detected
GIVEN source not in canonical Allman form
WHEN `green format --check` runs
THEN a format violation is reported

SCENARIO preprocessed input requires semantic-only
GIVEN `green check file.i`
WHEN no `--semantic-only` is supplied
THEN the request is rejected (exit 2)

SCENARIO safe fixes are semantics-preserving
GIVEN isolated `i++;` with a discarded result, missing braces, a provably
      redundant pointer cast, or a `NULL` spelling fix
WHEN `green fix` runs
THEN only the listed mechanical fixes are applied

SCENARIO unsafe rewrites are refused
GIVEN `&&`, `||`, `?:`, effectful expression restructuring, the comma
      operator, or nontrivial cast isolation
WHEN `green fix` runs
THEN no automatic rewrite is applied (programmer intent required)
