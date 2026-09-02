# Acceptance: outline

## Behavior green MUST exhibit

### 0030 - Single-block functions are accepted
GIVEN an owned function whose body is a single basic block
WHEN `green lint` runs
THEN no `green-outline` error is reported.

### 0031 - Already-outlined dispatchers are accepted
GIVEN a function reduced to the canonical dispatcher of single-block
helpers over an explicit program counter
WHEN `green lint` runs
THEN no `green-outline` error is reported.

### 0032 - Outlined output is green-clean
GIVEN a multi-block owned function outlined by `green outline`
WHEN the emitted translation unit is run through the full green check
THEN it passes `lint`, `matrix`, and `format`; emitted branch transfers
never use the `?:` operator; every controlled body is braced and Allman.

### 0033 - Outlining preserves semantics
GIVEN a multi-block function outlined by `green outline`
WHEN both the original and the emitted function run over generated inputs
THEN return values and observable side-effect order are identical.

### 0034 - Outlining is idempotent
GIVEN an already-outlined owned function
WHEN `green outline` runs
THEN the source is unchanged (re-outlining is a no-op).

## Behavior green MUST reject, avoid, or fail safely

### 0040 - Un-outlined control flow is flagged
GIVEN an owned function whose body contains more than one basic block that
is not the canonical dispatcher
WHEN `green lint` runs
THEN a `green-outline` error is reported at the function.

### 0041 - No hidden control in emitted output
GIVEN an outlining that would require a `?:` terminator
THEN green MUST emit an explicit `if`/`return` transfer instead; it MUST
NOT produce source that `green-hidden-control` would reject.

### 0042 - No bare single-statement or non-Allman control bodies
GIVEN emitted helpers and dispatchers
THEN every controlled body MUST use braces and Allman placement; the output
MUST be `format=pass` under the canonical profile.

### 0043 - No approximation of un-outlinable functions
GIVEN an owned function containing a VLA, asm, computed goto, address of a
label, block-scope static, recursion, setjmp/longjmp, or a rewrite that
touches a macro boundary
THEN green MUST report an explicit reason and MUST NOT transform the
function approximately.

### 0044 - No partial file modification
GIVEN a translation unit where any owned function cannot be outlined
WHEN `green outline` runs
THEN either the whole unit is outlined after validation or nothing is
written.

### 0045 - `main` is never outlined
GIVEN an owned `main` definition
THEN `green outline` leaves it unchanged.
