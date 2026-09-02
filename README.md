# green

A **strict C89 ∩ C23 source-profile toolchain**: one project source that
compiles warning-clean as strict C89 and strict C23, under both GCC and
Clang, with explicit semantic structure and one canonical Allman format.

`green` is a small policy layer over standard compiler infrastructure. It
orchestrates the GCC/Clang compiler matrix and clang-format, and adds a
clang-tidy plugin of focused semantic checks. It is not a compiler or a
standalone parser.

```text
GREEN = C89 ∩ C23 ∩ GCC-clean ∩ Clang-clean
        ∩ explicit-semantic-structure ∩ canonical-format
```

A project is **green** only when all required checks pass.

---

## What green checks

Every `.c` translation unit receives **seven checks**:

```text
                C89    C23
GCC             PASS   PASS
Clang           PASS   PASS
clang-tidy      PASS   PASS
format          PASS
```

That is: GCC in strict C89 and strict C23, Clang in strict C89 and strict
C23, the clang-tidy semantic suite in both standards, and canonical
formatting. All seven must pass.

### Semantic rules (clang-tidy plugin)

The governing rule: *expressions may calculate values but may not hide state
transitions, effects, sequencing, or value-dependent control flow.* The eleven
`green-*` checks implement it:

| Check | Rule |
| --- | --- |
| `green-hidden-control` | reject `&&`, `\|\|`, `?:`, and the comma operator in expression position |
| `green-transition-boundary` | assignment/update only as a complete statement or `for` clause; only prefix `++`/`--` |
| `green-effect-boundary` | an effectful call must form a complete transition |
| `green-pure-contract` | validate `GREEN_PURE`-marked functions (no visible mutation/effect/volatile) |
| `green-cast-boundary` | reject redundant, qualifier-discarding, integer/pointer and object/function-pointer casts; object-pointer conversions (incl. `void *`) allowed |
| `green-null` | `NULL` is the canonical null-pointer spelling |
| `green-declaration` | one object per declaration; prototype forms; `(void)` |
| `green-fallthrough` | no implicit fallthrough; exact `/* fall through */` marker |
| `green-preprocessor` | no function-like / token-manipulation macros; object-like macros may not hide control/transition at expansion |
| `green-toolchain-branching` | no compiler-identity conditionals |
| `green-flat` | inline work is welcome at a function's top level, but a *nested* block (`if`/`else`, loop, `switch`, bare `{}`) must be thin: no inline computation and at most one glue call per straight-line run; real work inside a decision/iteration region belongs in a worker |

`readability-braces-around-statements` (with `ShortStatementLines = 0`) is
reused for mandatory braces.

> **Worker/controller rule.** A function's top level is its own straight-line
> flow and may compute freely. But inside any nested block — an `if`/`else`
> body, a loop body, a `switch`/`case` body, or an explicit bare `{}` block —
> real computation is the smell: it should live in a worker function the code
> calls, so each decision/iteration site stays a single thin delegation. This
> keeps controllers and workers independently readable and testable.

---

## Requirements

- `gcc`
- `clang`
- `clang-tidy`
- `clang-format`
- A Clang development package providing `find_package(LLVM)` /
  `find_package(Clang)` for the build.

The `green-tidy` plugin is loaded into `clang-tidy` via `-load`. Clang
provides no ABI/API stability guarantee for out-of-tree plugins, so the
plugin must match the `clang-tidy` version it was built against. `green
doctor` reports a fatal error on a mismatch.

---

## Build

```sh
just setup     # configure the CMake build
just build     # build the `green` driver and the `green-tidy` plugin
```

Install layout:

```text
bin/green
lib/green/<clang-major>/green-tidy.so
share/green/clang-format.yaml
share/green/default-config.yaml
```

---

## Quick start

Create a `green.yaml` in the project root:

```yaml
version: 1

compile_commands:
    gcc: build/gcc/compile_commands.json
    clang: build/clang/compile_commands.json
```

Then run the full check:

```sh
green check
```

---

## Configuration

`green` reads `green.yaml` from the current directory. Relative paths
resolve from the directory containing `green.yaml`.

### `version`

The configuration schema version. Must be `1`.

### `compile_commands`

Two compilation databases describing the real compilation contexts for each
compiler:

```yaml
compile_commands:
    gcc: build/gcc/compile_commands.json
    clang: build/clang/compile_commands.json
```

`green` normalizes each command (preserving include/define/target semantics,
stripping `-std`/`-W`/`-o`/`-c`/codegen options), requires that the normalized
`.c` sets in both databases match, and applies its own strict profile with
`-fsyntax-only`. The original build remains authoritative for producing
binaries.

> `project_roots`, `exclude`, `compatibility_paths`, and `pure_functions`
> are enforced. Ownership of a source file is decided by `project_roots`
> (or, when none are configured, the primary translation unit's directory
> tree) and is never inferred from `-isystem`/system-header classification.

---

## Command reference

`green <command> [options]`

### `green check [file...]`

Run the full seven-check verification: toolchain validation, the GCC/Clang
matrix in C89 and C23, the clang-tidy suite in both standards, and format
verification. Prints a final matrix. With file arguments, checks only those
translation units (headers they include still receive diagnostics).

### `green matrix`

Run only the four compiler cells (GCC C89, GCC C23, Clang C89, Clang C23).

### `green lint [file...]`

Run only the clang-tidy semantic/preprocessor checks, in both C89 and C23,
without the compiler matrix.

### `green format --check`

Verify canonical formatting without modifying source. This is the supported
form. (`green format` without `--check` is verify-oriented in this release.)

### `green fix`

Apply mechanically semantics-preserving fix-its: missing braces,
discarded-result postfix→prefix, redundant pointer-cast removal, `NULL`
spelling, and formatting.

### `green semantic <file.i>`

Run semantic-only checks on a preprocessed `.i` file. Preprocessed input
cannot receive normative checks that require original source spelling.

### `green doctor`

Validate the toolchain: GCC/Clang/clang-tidy/clang-format paths and versions,
C89 and C23 capability, required-warning-option capability, compilation
database status, and the plugin's Clang ABI match. A plugin/`clang-tidy` ABI
mismatch is fatal.

### `--version`, `-V`

Print the version and the LLVM/Clang build versions.

### `--help`, `-h`

Print usage.

---

## Exit status

| Code | Meaning |
| --- | --- |
| `0` | fully green |
| `1` | source/profile violation |
| `2` | configuration or input error |
| `3` | required toolchain capability unavailable |
| `4` | internal `green` failure |

---

## Example

```text
$ green check
            C89    C23
GCC         PASS   PASS
Clang       PASS   PASS
clang-tidy  PASS   PASS

format      PASS

CGREEN      PASS
```

---

## Development and testing

```sh
just test       # unit tests + e2e fixture suite
just lint       # shellcheck + clang-format dry-run on the C++ sources
just check      # test + lint
just sanitize   # ASan + UBSan build; run the lint fixtures under both
just valgrind   # valgrind memcheck; degrades to UNSUPPORTED on this toolchain
```

The e2e suite lives in `tests/fixtures/` (121 passing and 132 failing
examples). Each fixture declares its expectations in a metadata header:

```c
/* green: lint=pass matrix=pass format=pass */
/* green: lint=green-hidden-control */
/* green: matrix=c23 */
/* green: format=fail */
```

A dimension is asserted only when tagged. See `.agent/` for the concept,
acceptance criteria, design, stories, and testing scenarios.

---

## Limitations

- The plugin requires a Clang/`clang-tidy` ABI match (see Requirements).
- `green format` is verify-oriented in this release; `green format --check`
  is the reliable form.
- On toolchains where valgrind cannot load the monolithic `libclang-cpp.so`,
  `just valgrind` reports UNSUPPORTED; memory-safety/UB coverage is provided
  by `just sanitize` (ASan + UBSan).
