# green — a strict C89 ∩ C23 source-profile toolchain

`green` defines and enforces a conservative C source profile with four goals:

1. the same project source compiles warning-clean as strict C89 and strict C23;
2. both GCC and Clang accept every translation unit;
3. source syntax exposes semantically relevant computation boundaries;
4. formatting has one canonical Allman-style representation.

The core acceptance relation:

```text
GREEN
    = C89
    ∩ C23
    ∩ GCC-clean
    ∩ Clang-clean
    ∩ explicit-semantic-structure
    ∩ canonical-format
```

A project is **green** only when all required checks pass.

C23 means ISO/IEC 9899:2024 mode selected with `-std=c23`. `green` green
means accepted by the selected compiler implementations, not formal
certification of complete ISO C23 implementation by those compilers.

`long long` / `unsigned long long` is admitted as a documented extension to
the strict C89 cell (via the `-Wno-long-long` baseline flag); all other
C99/C23-only syntax remains rejected by that cell.

## Governing semantic rule

```text
TERM          -> expression
TRANSITION    -> complete statement or typed for-clause
STRUCTURE     -> explicit statement/block structure
```

Expressions may calculate values but may not hide state transitions,
effects, sequencing, or value-dependent control flow.

green rejects semantic opacity, not unusual-looking C syntax. A meaningful
explicit cast or a pure object-like macro marks a semantic boundary rather
than hiding one.

## Components

```text
green               driver CLI
green-tidy          clang-tidy plugin
green-format.yaml   canonical clang-format profile
green.yaml          per-project configuration
```

GCC + Clang enforce language conformance and type/conversion correctness.
The clang-tidy plugin enforces semantic source discipline and
preprocessor policy. clang-format enforces canonical presentation.

## One translation unit receives seven checks

```text
1. GCC   C89
2. GCC   C23
3. Clang C89
4. Clang C23
5. tidy  C89
6. tidy  C23
7. format
```

All seven must pass for the project to be green.
