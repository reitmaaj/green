# Testing: interrupted build must not produce a hollow plugin

## Context

`green-tidy` is a MODULE shared object whose check registration lives in
`CgreenModule.cc` (a single static `ClangTidyModuleRegistry::Add` object).
If a build is interrupted while that translation unit compiles, the build
tree can hold a zero-byte object file that is newer than its source. A later
incremental build then relinks the plugin without the registration object:
the plugin dlopens cleanly but registers zero checks, so clang-tidy reports
`Error: no checks enabled` and every lint-tagged fixture silently degrades.

## Scenarios

SCENARIO an interrupted build cannot yield a silent no-check plugin
GIVEN a build tree containing a truncated or empty object for
      `CgreenModule.cc` that is newer than its source
WHEN `just build` runs and the e2e lint fixtures are asserted
THEN the plugin registers every `green-*` check
AND `clang-tidy -load=<plugin> -list-checks -checks=-*,green-*` lists the
      green module checks
AND the fixture suite fails loudly rather than passing vacuously when any
      green check is unregistered

SCENARIO the lint fixtures are the guard
GIVEN the e2e acceptance suite (`.agent/acceptance/0001-semantic.md`,
      `tests/harness/run_all.sh`)
WHEN any `green-*` check fails to register in the loaded plugin
THEN the first `lint=<check>` fixture aborts the suite with
      `FAIL: lint=<check> did not fire in both modes`
AND no fixture can pass vacuously for want of a registered check
