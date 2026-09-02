# Testing: outline

SCENARIO straight-line function is accepted
GIVEN a function whose body is a single basic block with no internal
control flow
WHEN `green lint` runs
THEN no `green-outline` error is reported

SCENARIO already-outlined dispatcher is accepted
GIVEN a function reduced to the canonical `for (;;) { switch (pc) {...} }`
dispatcher over single-block helpers
WHEN `green lint` runs
THEN no `green-outline` error is reported

SCENARIO un-outlined `if`/`else` function is flagged
GIVEN a function whose body contains an `if`/`else` (two basic blocks)
WHEN `green lint` runs
THEN a `green-outline` error is reported at the function

SCENARIO un-outlined loop function is flagged
GIVEN a function whose body contains a `while`/`for`/`do` loop
WHEN `green lint` runs
THEN a `green-outline` error is reported at the function

SCENARIO nested control flow is flagged once per function
GIVEN a function with nested loops and conditionals
WHEN `green lint` runs
THEN exactly one `green-outline` error is reported for that function

SCENARIO outlined output conforms to green
GIVEN a multi-block function outlined by `green outline`
WHEN the emitted translation unit is run through `green lint`,
`green matrix`, and `green format`
THEN all checks pass with no `?:` terminator and Allman mandatory braces

SCENARIO re-outlining is a no-op
GIVEN an already-outlined translation unit
WHEN `green outline` runs
THEN the file is unchanged

SCENARIO un-outlinable construct is reported, never approximated
GIVEN an owned function containing a VLA, asm, computed goto, address of a
label, block-scope static, or macro-boundary rewrite
WHEN `green outline` runs
THEN the function is reported with an explicit reason and the file is
unchanged

SCENARIO no partial output on failure
GIVEN a translation unit where some owned function cannot be outlined
WHEN `green outline` runs
THEN nothing is written to the file and the failure is reported
