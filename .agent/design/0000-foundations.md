# Foundations

## Acceptance model

```text
GREEN = C89 ∩ C23 ∩ GCC-clean ∩ Clang-clean
        ∩ explicit-semantic-structure ∩ canonical-format
```

Each `.c` translation unit passes seven checks: GCC C89, GCC C23, Clang C89,
Clang C23, tidy C89, tidy C23, format.

## Language-matrix truth

The C89 cell is the hard ceiling. C99/C23-only syntax (mixed declarations
and statements, `//` comments, `for (int i = 0; ...)`, `nullptr`,
`[[fallthrough]]`) fails the C89 compiler cell by construction and therefore
can never be green. The one documented exception is `long long` / `unsigned
long long`, admitted by the `-Wno-long-long` baseline flag; the rest of the
C99/C23-only syntax remains rejected. The clang-tidy checks must not duplicate
what the compiler matrix already rejects; they focus on cross-standard
semantic discipline.

## Compiler profile

Every compiler invocation replaces its diagnostic/language profile with the
canonical `green` profile:

```text
-std=c89 | -std=c23
-pedantic-errors -Wall -Wextra -Werror
-Wconversion -Wsign-conversion
-Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition
-Wundef -Wshadow -Wformat=2 -Wcast-qual
-Wno-long-long
-fsyntax-only
```

`-Wconversion` and `-Wsign-conversion` enforce the rule that potentially
lossy implicit numeric conversions cannot silently cross representation
domains; explicit casts identify intentional conversion boundaries.

`-Wno-long-long` relaxes the strict C89 cell only: `long long` / `unsigned
long long` (a C99 type) is accepted instead of rejected by
`-pedantic-errors`. Every other pedantic and conversion warning stays
enforced, so it is a narrow, documented extension over pure ISO C89.

The driver MUST verify that the selected compiler accepts every required
option. A missing required capability makes the toolchain unsupported;
`green` MUST NOT silently omit the check.

## Exit status

```text
0    fully green
1    source/profile violation
2    configuration/input error
3    required toolchain capability unavailable
4    internal green failure
```
