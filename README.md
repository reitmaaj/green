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
| `green-outline` | every basic block must be outlined to a file-local helper; only straight-line, leaf, and canonical `for(;;){switch(pc)}` dispatcher bodies are accepted |

`readability-braces-around-statements` (with `ShortStatementLines = 0`) is
reused for mandatory braces.

> **Outline rule.** `green-outline` is a *block-level* discipline layered on
> top of the expression rules: a function may reach for control flow only as
> far as one terminal transfer (a final `return`, an else-less
> `if (cond) { return A; }` followed by `return B;`, or the canonical
> `for (;;) { switch (pc) { ... } }` dispatcher over single-block helpers).
> A body that still contains an un-extracted `if`/`else`, a `while`/`for`/`do`
> loop, a `switch`, or several sequential decisions is reported by
> `green-outline` because its basic blocks should be extracted into file-local
> helpers. `green outline` performs that extraction automatically and its
> output is itself green-clean (no hidden control, mandatory Allman braces,
> canonical formatting).

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
just build     # build the `green` driver, `green-outline`, and the plugin
```

Install layout:

```text
bin/green
bin/green-outline
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

### `green outline [file...]`

Extract the basic blocks of every function that still contains un-extracted
control flow into file-local helpers driven by an explicit program counter
(the shape `green-outline` enforces). Already-outlined translation units are
reported and left untouched, so repeated runs are a no-op. The emitted source
is formatted to the canonical profile and re-verified: it must pass `lint`,
`matrix`, and `format`. Runs in place; back up before applying to a live tree.
(With no file arguments, processes the whole project.)

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
just test       # unit tests + 200-fixture e2e suite
just lint       # shellcheck + clang-format dry-run on the C++ sources
just check      # test + lint
just sanitize   # ASan + UBSan build; run the lint fixtures under both
just valgrind   # valgrind memcheck; degrades to UNSUPPORTED on this toolchain
```

The e2e suite lives in `tests/fixtures/` with 100 passing and 100 failing
examples. Each fixture declares its expectations in a metadata header:

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
