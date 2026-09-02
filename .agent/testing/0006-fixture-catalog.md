# Testing: e2e fixture catalog (100 pass + 100 fail)

Each fixture in `tests/fixtures/pass|fail/NNN-<dim>-<case>.c` carries a metadata
header `/* green: lint=... matrix=... format=... */`. A dimension is asserted
only when tagged.

- `lint=pass` / `lint=<check>` : clang-tidy, asserted under C89 and C23.
- `matrix=pass` / `matrix=c89|c23` : gcc+clang with the strict green baseline.
- `format=pass` / `format=fail` : clang-format dry-run against the installed profile.

## Pass (100)

- lint (70) covering every semantic rule: pure terms and nesting, complete
  transitions (assign/compound/prefix), for-clauses, result-binding effect
  calls, `GREEN_PURE`-marked and pure calls, intentional numeric casts,
  `void*` implicit conversion, `NULL` usage, single-object declarations,
  prototypes/`(void)`, braced control (incl. else-if, empty blocks), explicit
  fallthrough, object-like macros and includes, toolchain-neutral source.
- matrix (15): dialect-neutral, `-Wconversion/-Wsign-conversion`-clean under
  both C89 and C23 with both compilers.
- format (15): canonical Allman/4-space/80-column presentation.

## Fail (100)

- lint (70), each attributed to exactly one expected check:
  - green-hidden-control: `&&`, `||`, `?:`, comma operator in conditions,
    assignments, returns, for/while conditions.
  - green-transition-boundary: embedded assignment/update, postfix `++`/`--`
    (statement, argument, index, condition, return), nested assignment.
  - green-effect-boundary: effect call inside arithmetic, argument, condition,
    return, index, binary op, and initializers.
  - green-pure-contract: marked function with global mutation, effect call,
    volatile access, unproven call.
  - green-cast-boundary: redundant same-type, redundant `void*`, integer<->pointer,
    object punning, object<->function pointer, const-qualifier discard.
  - green-null: `0`, `(void *)0`, `(struct node *)0`, `return 0`.
  - green-declaration: multi-declarator (local/file/struct), empty `()`,
    pointer multi-declarator.
  - green-fallthrough: implicit fallthrough, fallthrough through a branch,
    wrong marker spelling.
  - green-preprocessor: function-like macro, token pasting, stringification,
    macro-introduced control and `return`.
  - green-toolchain-branching: `__GNUC__`, `__clang__`, `__STDC_VERSION__`,
    `defined(__GNUC__)`.
  - green-flat: inline computation inside a nested block (`if`/`else`, loop,
    `switch`, or bare `{}`) — see `flat-*.c` fixtures.
  - readability-braces-around-statements: unbraced if/else/while/for/do,
    null-statement empty body, unbraced else branch.
- matrix (15): C99/C23-only syntax rejected by the C89 cell (`//` comment,
  for-loop declaration, mixed declarations, `long long`, designated
  initializers, `inline`, `_Bool`, hex float) and C23-only keyword-as-identifier
  rejected by the C23 cell (`typeof`, `typeof_unqual`, `true`, `false`, `bool`,
  `nullptr`, `alignas`).
- format (15): non-canonical presentation that clang-format rewrites.

## Language-mode dependent constructs

K&R definitions and unspecified-parameter `()` declarations are C89-only:
- `()` is caught by green-declaration in both modes via source-spelling
  detection (C23 treats `()` as `(void)`).
- K&R definitions fail strict C89's `-Wold-style-definition` and are rejected
  by C23's parser, so they are not lint fixtures; they are covered by matrix
  fixtures via C23-keyword cases instead.

## Sanitizer interplay

`just sanitize` runs only the lint-tagged fixtures (under ASan/UBSan, C89 and
C23); matrix and format fixtures are excluded because they do not fit the
zero-or-targeted-diagnostic model.
