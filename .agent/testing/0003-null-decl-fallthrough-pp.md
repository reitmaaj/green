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

SCENARIO pure object-like macro is accepted
GIVEN a project object-like macro whose replacement is a pure value, type,
      or token abstraction, e.g. `#define BAD_VALUE (((j89_len) - 1))`,
      `#define MASK (A | B)`, `#define NIL ((void *)0)`, or
      `#define SZ sizeof(struct { int x; })`
WHEN `green lint` runs
THEN no error is reported

SCENARIO object-like macro hiding control is rejected at expansion
GIVEN a project object-like macro whose expansion produces hidden control or
      transition structure, e.g. `#define CHOOSE (a ? b : c)`,
      `#define BOTH (a && b)`, `#define UPDATE (x += 1)`, or
      `#define RETURN return`
WHEN `green lint` runs
THEN a `green-preprocessor` error is reported, attributed to the macro

SCENARIO macro control attribution walks the expansion ancestry
GIVEN `#define A (x && y)` and `#define B A` where both are owned
WHEN `green lint` runs
THEN a `green-preprocessor` error is reported, attributed to the innermost
     owned macro `A`

SCENARIO owned macro wrapping an external control macro is flagged
GIVEN `#define OUTER SOME_EXTERNAL_MACRO` where the external macro expands to
      hidden control
WHEN `green lint` runs
THEN a `green-preprocessor` error is reported, attributed to `OUTER`

SCENARIO ownership is governed by project roots, not compiler classification
GIVEN a project-owned header included via `-isystem` that contains an
      object-like macro with a pure term replacement
WHEN `green lint` runs
THEN no error is reported; the header is owned by path, not masked by
     `-isystem`

SCENARIO compiler-identity branching is rejected
GIVEN project source using `__STDC_VERSION__`, `__GNUC__`, or `__clang__`
WHEN `green lint` runs
THEN a `green-toolchain-branching` error is reported

SCENARIO compatibility-path exemption
GIVEN a file under `compatibility_paths` using compiler-identity or
      representation-cast code
WHEN `green lint` runs
THEN the enumerated exemption is honored
