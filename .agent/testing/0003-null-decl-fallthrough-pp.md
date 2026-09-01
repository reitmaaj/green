# Testing: null, declarations, fallthrough, preprocessor, branching

SCENARIO NULL is the canonical null spelling
GIVEN `p = NULL;` or `if (p == NULL) { }`
WHEN `green lint` runs
THEN no error is reported

SCENARIO alternate null spellings are rejected
GIVEN `p = 0;` `p = (void *)0;` or `p = (struct node *)0;`
WHEN `green lint` runs
THEN a `green-null` error is reported

SCENARIO NULL macro expansion is not source spelling
GIVEN a `(void *)0` that is the expansion of the implementation's `NULL`
WHEN `green lint` runs
THEN no error is reported (SourceManager distinguishes expansion)

SCENARIO one declaration declares one object
GIVEN `int count, index;` or `char *name, buffer[32];`
WHEN `green lint` runs
THEN a `green-declaration` error is reported

SCENARIO prototypes are mandatory
GIVEN a K&R definition or an unspecified-parameter declaration
WHEN `green lint` runs
THEN a `green-declaration` error is reported

SCENARIO zero-argument prototype is canonical
GIVEN `int poll_status(void);`
WHEN `green lint` runs
THEN no error is reported

SCENARIO implicit fallthrough is rejected
GIVEN a `switch` case that flows into the next without a marker
WHEN `green lint` runs
THEN a `green-fallthrough` error is reported

SCENARIO explicit fallthrough marker is accepted
GIVEN exactly `/* fall through */` after the last statement of a case
WHEN `green lint` runs
THEN no error is reported

SCENARIO function-like macros are rejected
GIVEN a project-defined `#define MAX(a, b) ...`
WHEN `green lint` runs
THEN a `green-preprocessor` error is reported

SCENARIO token-manipulation macros are rejected
GIVEN token pasting or stringification macros
WHEN `green lint` runs
THEN a `green-preprocessor` error is reported

SCENARIO compiler-identity branching is rejected
GIVEN project source using `__STDC_VERSION__`, `__GNUC__`, or `__clang__`
WHEN `green lint` runs
THEN a `green-toolchain-branching` error is reported

SCENARIO compatibility-path exemption
GIVEN a file under `compatibility_paths` using compiler-identity or
      representation-cast code
WHEN `green lint` runs
THEN the enumerated exemption is honored
