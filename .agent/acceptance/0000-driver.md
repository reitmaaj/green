# Acceptance: driver, matrix, and exit status

## Mandatory behaviors

- `green check` in a project whose source is compliant returns exit 0.
- `green check` runs GCC C89, GCC C23, Clang C89, Clang C23, clang-tidy
  C89, clang-tidy C23, and format verification.
- `green check` prints a final matrix reporting PASS/FAIL per cell.
- `green check <file>` restricts checking to the named translation units
  while still diagnosing headers they include.
- `green format --check` verifies canonical formatting without modifying
  source; `green format` rewrites source.
- `green lint` runs clang-tidy in both standard modes without the compiler
  matrix.
- `green matrix` runs only the four compiler cells.
- `green semantic file.i` runs semantic-only checks on preprocessed input.
- `green check file.i` without `--semantic-only` is rejected (exit 2).
- `green doctor` reports paths, versions, C89/C23 capability, warning-option
  capability, plugin ABI match, compilation-database status, and format
  compatibility; a plugin ABI mismatch is fatal.
- `green fix` applies only mechanically semantics-preserving fix-its.

## Unacceptable behaviors

- `green` MUST NOT silently omit a required warning option when the selected
  compiler lacks it; it reports the toolchain as unsupported (exit 3).
- `green` MUST NOT proceed when the normalized GCC and Clang translation-unit
  sets do not match; it reports a configuration failure (exit 2).
- `green` MUST NOT invent a standalone semantic interpretation of a header.
- `green` MUST NOT report exit 0 when any of the seven checks fail.

## Exit status

- 0 fully green; 1 source/profile violation; 2 configuration/input error;
  3 required toolchain capability unavailable; 4 internal green failure.

## Exit-status mapping fixtures

- exit 1 on a source violation found by any check.
- exit 2 on a missing/invalid `green.yaml` or compile-DB mismatch.
- exit 3 when a required compiler option is unsupported.
- exit 4 on an internal driver failure (e.g. unhandled invariant).
