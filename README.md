# green

A **strict C89 ∩ C23 source-profile toolchain**: one project source that
compiles warning-clean as strict C89 and strict C23, under both GCC and
Clang, with explicit semantic structure and one canonical Allman format.

`green` is a small policy layer over standard compiler infrastructure. It
orchestrates the GCC/Clang compiler matrix and clang-format, and adds a
`clang-tidy` plugin of focused semantic checks. It is **not** a compiler and
**not** a standalone parser; every language check is delegated to a
real, installed toolchain (`gcc`, `clang`, `clang-tidy`, `clang-format`).

```text
GREEN = C89 ∩ C23 ∩ GCC-clean ∩ Clang-clean
        ∩ explicit-semantic-structure ∩ canonical-format
```

A project is **green** only when *all* required checks pass on every
translation unit it owns.

> "C23" means ISO/IEC 9899:2024 mode selected with `-std=c23`. "green" here
> means *accepted by the selected compiler implementations under that mode*,
> not formal certification that those compilers fully implement ISO C23.

---

## Table of contents

- [What green is](#what-green-is)
- [The seven checks](#the-seven-checks)
- [Semantic rule and checks (clang-tidy plugin)](#semantic-checks)
- [Components](#components)
- [Requirements](#requirements)
- [Build and install](#build-and-install)
- [Configuration (`green.yaml`)](#configuration)
- [Ownership model](#ownership)
- [Compilation database normalization](#compile-database-normalization)
- [Command reference](#command-reference)
- [Interpreting output and exit status](#interpreting-output-and-exit-status)
- [A worked example](#worked-example)
- [Development and testing](#development-and-testing)
- [Limitations](#limitations)

---

## What green is

`green` defines a conservative C source profile with four goals:

1. The same project source compiles warning-clean as strict C89 **and**
   strict C23.
2. Both GCC and Clang accept every translation unit.
3. Source syntax exposes semantically relevant computation boundaries
   (state transitions, effects, sequencing, and value-dependent control flow
   may not hide inside expressions).
4. Formatting has exactly one canonical Allman-style representation.

The two compilers enforce language conformance and type/conversion
correctness; the `clang-tidy` plugin (`green-tidy`) enforces *semantic source
discipline* and *preprocessor policy*; `clang-format` enforces canonical
presentation. A source unit only becomes green when it survives all of them.

### Governing semantic rule

```text
TERM       -> expression
TRANSITION -> complete statement or typed for-clause
STRUCTURE  -> explicit statement/block structure
```

Expressions may calculate values but may not hide state transitions,
effects, sequencing, or value-dependent control flow.

`green` rejects **semantic opacity**, not unusual-but-harmless C syntax. A
meaningful explicit cast or a pure object-like macro marks a semantic
boundary; it does not hide one.

---

## The seven checks

Every `.c` translation unit receives seven checks:

```text
1. GCC   C89          (strict)
2. GCC   C23          (strict)
3. Clang C89          (strict)
4. Clang C23          (strict)
5. tidy  C89          (semantic + preprocessor, both standards)
6. tidy  C23
7. format             (canonical Allman clang-format)
```

All seven must pass for the project to be green. `green check` runs all
seven and prints a result matrix; `green matrix`, `green lint`,
`green format --check` run subsets (see [Command reference](#command-reference)).

The compiler cells are `-fsyntax-only`: they parse and type-check under the
green baseline flags but produce no object code. The original build system
remains authoritative for producing binaries; green only verifies the source
profile.

### The compiler baseline

Each compiler cell runs with the translation unit's own compile-command
arguments (includes, defines, target semantics) plus an enforced strict
profile. The profile flags (`share/...`/`src/driver/CompileDB.cc`,
`greenBaseline`) are:

```text
-pedantic-errors -Wall -Wextra -Werror
-Wconversion -Wsign-conversion -Wstrict-prototypes
-Wmissing-prototypes -Wold-style-definition
-Wundef -Wshadow -Wformat=2 -Wcast-qual
-fsyntax-only
```

Pre-existing `-std`, `-W*`, optimization, debug, and codegen options from the
compilation database are stripped and replaced by `-std=c89` / `-std=c23`
plus this baseline (see [Compilation database normalization](#compile-database-normalization)).

---

## Semantic checks

Eleven `green-*` checks plus one reused built-in check implement the semantic
rule. They are loaded as the `green-tidy` plugin into `clang-tidy`.

| Check | Rule (short) |
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
| `green-flat` | a *nested* block must be thin; inline computation belongs in a worker |

`readability-braces-around-statements` (with `ShortStatementLines = 0`) is
reused for mandatory braces.

> **Two kinds of explicitness.** Green enforces explicitness at two
> independent axes. **Expression transparency** keeps effects, mutation,
> sequencing, and value-dependent control from hiding inside expressions
> (`green-hidden-control`, `green-transition-boundary`,
> `green-effect-boundary`). **Structural transparency** gives every
> nontrivial computation reached through control flow a named function
> boundary (`green-flat`). A value category ladder captures this: TERM
> (ordinary value), TRANSITION (explicit mutation), EFFECT (explicit
> call/binding boundary), CONTROL (explicit braced control), WORKER (named
> unit with substantive computation), and CONTROLLER REGION (nested block with
> only thin orchestration). This explains why Green-produced code reads unlike
> idiomatic C: it deliberately exchanges local concision for explicit semantic
> and test boundaries.

> V1 provides **no inline `NOLINT` escape**. Every owned location is subject
> to the checks; if a check is wrong for your code the fix is to change the
> code or the ownership configuration, not to suppress one location.

### `green-hidden-control`

Rejects short-circuit operators, the conditional operator, and the comma
operator in *expression position* — places that hide value-dependent control
flow inside a value.

- PASS: arithmetic/comparison/bitwise/logical-not expressions; nested pure
  computation (`x = ((a + b) * (c - d)) + a * d;`).
- FAIL: `if (a && b) { ... }`, `x = cond ? a : b;`, `r = a || b;`,
  `for (...; a , b; ...)` when `,` is an operator.
- Accepted: the *separator* comma between arguments, declarators,
  initializer elements, or `for` clauses is not the comma operator.

### `green-transition-boundary`

Assignment and update must be a complete transition: a standalone statement
or a `for`-clause. Only prefix `++`/`--` is allowed; postfix is always
rejected, and an assignment/update may not be *embedded* inside another
expression.

- PASS: `x = y;`, `x += n;`, `++i;`, `--i;`, `for (i = 0; i < n; ++i)`.
- FAIL: `x = ++i;`, `f(i++);`, `x = (y = z);`, `x += (y += z);`,
  `a[i++] = value;`, postfix `++`/`--` in any position, an effectful
  initializer `int fd = open_file(path);`, and a single `for` clause carrying
  more than one transition.

### `green-effect-boundary`

An effectful call (one not proven pure) must itself form a complete
transition: a standalone discarded statement, or the sole right-hand side of
an assignment (a result binding). An effect may not be buried inside a larger
expression.

Parentheses and implicit or explicit casts are **placement-transparent**
wrappers around a call: an assignment whose right-hand side is the call plus
any wrapping parens/casts is still a valid result binding. This transparency
implies no approval of an explicit cast — cast admissibility is judged solely
by `green-cast-boundary` (see below). An **indirect** call (through a
function pointer) is conservatively effectful and obeys the same rule.

The complete-transition test is measured on the **outermost** expression, so a
transparently-wrapped call that forms an *entire* statement or clause — a
parenthesized `(f());`, an explicit `(void)f();` discard, or an effectful call
as a `for` increment — is a complete transition, while the same wrapper does
not help once the call feeds a larger computation.

- PASS: `consume(x);` alone; `r = produce();` (result binding);
  `p = allocate();` where `allocate` returns `void *` (Clang inserts an
  implicit `void * -> struct foo *` conversion) or
  `p = (struct foo *)allocate();`; `(void)produce();`; `(produce());`;
  `for (...; ...; step())` (effectful increment).
- FAIL: `n = produce() + 1;`, `consume(produce());`, `if (produce()) {}`,
  `return produce();`, an effectful call in a `while`/`do`/`for` condition,
  and consuming or computing from a wrapped result —
  `x = -produce();`, `x = *produce();`, `x = produce()[0];`,
  `x = (f()) + 1;`.
- An effect-placement diagnostic is never produced merely because a cast chain
  contains a representation escape (e.g. through `uintptr_t`); only the bad
  cast is reported.

### `green-pure-contract`

A function annotated with the `GREEN_PURE` marker (or listed in
`pure_functions`) must truly be pure: no visible state mutation, no effectful
call, no volatile access, and no other unproven (not themselves pure) call.

- PASS: a `GREEN_PURE`-marked function whose body is pure arithmetic.
- FAIL: a marked function whose body mutates state, calls an effect, reads a
  `volatile`, or calls another function that has not been proven pure.
- FAIL: `GREEN_PURE` not on its own token line, or not immediately preceding
  a declaration/definition; a false (unearned) purity annotation.

`GREEN_PURE` is detected as an object-like macro whose expansion occupies its
own token line immediately before a function, plus any names in the
`pure_functions` list.

A `GREEN_PURE` marker on a *prototype* (a declaration without a body) asserts
purity of an external function, so calls to it are usable in expression
position; like `pure_functions`, this is unvalidated trust (there is no body
to check), whereas a marked *definition* is validated by this check.

### `green-cast-boundary`

Casts are allowed when they mark a real, intentional domain or width
boundary; they are rejected when they are redundant or discard type
information.

A cast (implicit or explicit) around an effect result is *placement
transparent* for `green-effect-boundary`; whether that cast is admissible is
decided **here**, and only here.

- PASS: intentional narrowing/signedness/domain casts, e.g. guarded
  `(unsigned int)size`, `(unsigned char)value`, `(long)int_value`.
- PASS: `void *` conversions in both directions, written or implied
  (`(struct node *)raw`, `(void *)node`), and unrelated object-pointer
  conversions (`pb = (struct b *)pa;`). Access through a converted pointer is
  governed by C's effective-type/aliasing rules, not by this cast check.
- FAIL: a cast from a type to its same canonical type (redundant cast), casts
  between integers and pointers, object-to-function-pointer casts, and casts
  that discard `const`/`volatile`.

### `green-null`

`NULL` is the canonical spelling of the null pointer constant.

- PASS: `p = NULL;`, `if (p == NULL)`.
- FAIL (stylistic): `p = 0;`, `p = (void *)0;`, `p = (struct node *)0;`.
- PASS: a `(void *)0` that arises from the *expansion* of the
  implementation's own `NULL` macro is distinguished by source spelling
  (SourceManager) and accepted.

### `green-declaration`

Declarations must be explicit and singular.

- PASS: one object per declaration; prototype forms; `int poll_status(void);`.
- FAIL: `int count, index;`, `char *name, buffer[32];` — for locals, file
  scope, and struct/union fields.
- FAIL: K&R definitions and unspecified-parameter (`()`) declarations.
- Not applied to: function parameter lists.

### `green-fallthrough`

`switch` cases must not fall through implicitly; intentional fall-through
must be marked with the exact canonical marker.

- PASS: `break`-terminated cases; the canonical `/* fall through */` marker as
  the last statement before the next `case`/`default`.
- FAIL: implicit fallthrough without a marker.
- FAIL: any marker other than the exact canonical spelling
  (e.g. `/* fallthrough */`, `// fall through`, `/* FALLTHROUGH */`).

### `green-preprocessor`

Preprocessor use must be transparent: it may abstract *values and tokens* but
may not inject hidden control/transition structure or do token surgery.

- PASS: `#include`, ordinary object-like macros, include guards.
- PASS: project object-like macros whose expansion is a pure value/type/token
  abstraction — arithmetic, bitwise, `sizeof(struct { ... })`, `void *`
  constants. The empty `GREEN_PURE` annotation is exempt.
- FAIL: project-defined **function-like** macros.
- FAIL: token-pasting and stringification (`##`, `#`) macros.
- FAIL: object-like macros whose expansion hides control or transition
  structure (`?:`, `&&`, `||`, assignment, `return`, statement blocks around
  computation). Attribution walks the macro-expansion ancestry and reports the
  innermost owned object-like macro.
- Applies only to project-owned files (see [Ownership](#ownership)).

### `green-toolchain-branching`

Source that selects behavior based on compiler identity is rejected, so that
the one source is genuinely accepted by both GCC and Clang.

- FAIL: `#ifdef __GNUC__`, `#if defined(__clang__)`,
  `#if __STDC_VERSION__ >= ...`, or other compiler/version-identity macros in
  project source outside configured compatibility paths.
- PASS: such tests inside `compatibility_paths`; system/third-party headers
  are exempt by ownership.

### `green-flat`

`green-flat` is a **structural decomposition / test-boundary discipline**:
every nontrivial computation reached through control flow should acquire a
named function boundary so it is independently callable, testable, and
analyzable. Inline work is welcome at a function's **top level** — its own
straight-line flow may compute freely. It is the smell only when it sits
inside a **nested** block: an `if`/`else` body, a loop body, a
`switch`/`case` body, or an explicit bare `{}` block.

Every nested block must be **thin**: no inline computation, and at most **one
glue statement per straight-line run**. A *glue* statement is a discarded
call, a prefix `++`/`--`, a constant init/assignment, or a call-result
binding. Real work inside a decision/iteration region belongs in a *worker
function* the code calls — nested control blocks orchestrate; worker
functions compute.

- PASS: top-level computation; a pure straight-line worker with no nested
  block; a control body that holds a single glue statement (`step();`,
  `x = pull();`, `--n;`, `int z = pull();`); controllers that delegate work
  to a worker per decision site.
- FAIL: any operator-built value inside a nested body — `s = s + i;`,
  `s += n;`, `int y = n * i;` — in an `if`/`else` branch, loop body, or bare
  block.
- FAIL: a straight-line run of more than one glue statement inside a nested
  body (`a(); b();`, `step(); --n;`, two bindings).
- Note: a `switch` case body is inspected only when the case forms its own
  nested `{}` block; unbraced `case` statements are not inline-scanned by the
  current check.

> **Worker/controller rule.** A function's top level is its own straight-line
> flow and may compute freely. But inside any nested block — an `if`/`else`
> body, a loop body, a `switch`/`case` body, or an explicit bare `{}` block —
> real computation is the smell: it should live in a worker function the code
> calls, so each decision/iteration site stays a single thin delegation. This
> keeps controllers and workers independently readable and testable. A loop
> body that reduces to a single delegation (e.g. `hm_i_collect_free(...)`) is
> a successful Green transformation.

### Reused: `readability-braces-around-statements`

Every controlled body (`if`/`else`/`while`/`do`/`for`) uses braces; empty
bodies use empty blocks (`{}`), never null statements; `else if` remains a
chain. Configured with `ShortStatementLines = 0` so nothing is ever allowed
to omit braces.

---

## Components

```text
green               driver CLI            (bin/green)
green-tidy          clang-tidy plugin     (lib/green/<clang-major>/green-tidy.so)
clang-format.yaml   canonical format profile (share/green/clang-format.yaml)
default-config.yaml documented configuration template (share/green/default-config.yaml)
green.yaml          per-project configuration (must be created by the user)
```

The driver locates and drives `gcc`, `clang`, `clang-tidy`, and
`clang-format` on `PATH`, and loads the `green-tidy` plugin into
`clang-tidy`.

---

## Requirements

Runtime tools, all discoverable on `PATH`:

- `gcc`
- `clang`
- `clang-tidy` (must match the Clang/LLVM version green was built against)
- `clang-format`

Build-time requirements:

- A C++17 compiler and CMake ≥ 3.20.
- A Clang development package providing `find_package(LLVM)` /
  `find_package(Clang)` for the build.

### Plugin / clang-tidy ABI match

The `green-tidy` plugin is loaded into `clang-tidy` via `-load`. Clang
provides **no ABI/API stability guarantee** for out-of-tree plugins, so the
plugin must be built against (and loaded into) the same Clang/LLVM version.

`green doctor` reports the plugin's LLVM/Clang build version; a
plugin/`clang-tidy` ABI mismatch is fatal (exit status 3). Build green with a
Clang/LLVM that matches the `clang-tidy` you run.

---

## Build and install

```sh
just setup     # cmake -S . -B .agent/tmp/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
just build     # build the `green` driver and the `green-tidy` plugin
```

Or with plain CMake:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Install layout (`install(TARGETS ...)`):

```text
bin/green
lib/green/<clang-major>/green-tidy.so
share/green/clang-format.yaml
share/green/default-config.yaml
```

The build embeds the plugin path and format profile path as compile-time
definitions (`GREEN_PLUGIN_PATH`, `GREEN_FORMAT_PATH`), so the built driver
already knows where its plugin and profile live.

Optional sanitizer build for validating green itself:

```sh
cmake -S . -B build-san -DGREEN_SANITIZE=address,undefined
cmake --build build-san -j
```

---

## Configuration

`green` reads `green.yaml` from the **current working directory**. Run green
from the directory that holds your `green.yaml`. The directory is the base
for resolving relative paths in the file.

```yaml
version: 1

compile_commands:
    gcc: build/gcc/compile_commands.json
    clang: build/clang/compile_commands.json

project_roots:
    - src
    - include

exclude:
    - build/**
    - vendor/**
    - third_party/**

compatibility_paths:
    - src/compat/**

pure_functions: []
```

A documented template is installed at `share/green/default-config.yaml`.

### Schema reference

- `version` (required) — the configuration schema version. Must be `1`.
- `compile_commands` (required) — two compilation databases describing the
  real compilation contexts for each compiler:
  - `gcc`: path to the GCC compilation database.
  - `clang`: path to the Clang compilation database.
  Relative paths resolve against the directory holding `green.yaml`.
- `project_roots` — list of directories whose sources are owned and checked.
- `exclude` — list of directories to skip even if under a project root.
- `compatibility_paths` — list of directories exempt from
  `green-toolchain-branching` (compiler-identity conditionals are permitted
  there).
- `pure_functions` — list of function names treated as pure for
  `green-effect-boundary` / `green-pure-contract` purposes, in addition to
  the `GREEN_PURE` marker. The driver serializes this list into every check's
  options, so it must reach the checks that consume it.

Each of `project_roots`, `exclude`, `compatibility_paths`, and
`pure_functions` is a **block list**: it is opened by its own top-level
header key and populated by the indented `- item` lines beneath it (or by an
inline `[...]` form such as `pure_functions: []`). A subsequent header starts
a new list; it does not invalidate or swallow its predecessor.

Unknown keys, a missing `version`, an unsupported `version`, missing
`compile_commands.gcc`/`clang` entries, or an **orphaned** `- item` line that
appears with no active block-list header are **configuration errors** (exit
status 2).

> The schema is intentionally small and fixed. The sample project in
> `tests/driver/sample/green.yaml` shows the minimal form.

---

## Ownership

Which files receive diagnostics is decided by **ownership**, never by
compiler header classification (`-isystem`, system-header bits, angle vs
quoted include). This is deliberate: a header under `project_roots` is
checked whether or not the toolchain happens to treat it as a system header.

```text
if project_roots is nonempty:
    owned(file) := beneath(canonical(file), any project_root)
else:
    owned(file) := beneath(canonical(file), canonical(dirname(primary TU)))

owned(file) := owned(file) && !beneath(canonical(file), any exclude)
```

- Containment compares **path components**, not string prefixes:
  `/project/src` owns `/project/src/x.c` but not `/project/src-old/x.c`.
- Paths are canonicalized.
- Source locations that cannot resolve to a real source file (virtual,
  builtin, scratch buffers) are simply not owned.
- Ownership applies per check through options repeated across every `green-*`
  check (the driver builds the clang-tidy `--config` JSON from the four
  ownership lists).

Every `green-*` check reports only owned locations.

---

## Compile database normalization

A compilation database entry carries `file`, `directory`, and a `command`
(string) or `arguments` (array). green:

1. Tokenizes the command, honoring quotes and backslash escapes.
2. Keeps only entries whose `file` ends in `.c`.
3. Drops the compiler driver name (first token).
4. **Strips** normalization-irrelevant arguments:
   - any `-std=...` (replaced by the cell's standard),
   - `-c`, `-fsyntax-only`, `-E`,
   - `-o <file>` and its argument,
   - any `-W*` flag (replaced by the green baseline),
   - dependency flags `-MD -MMD -MP -MG -MF -MT -MQ` and their arguments,
   - debug/optimization flags `-g -O0 -O1 -O2 -O3 -Os -Ofast`.
5. Preserves include/define/target semantics (`-I`, `-D`, `-U`, target flags,
   etc.).
6. Requires that the normalized set of `.c` files in the GCC database and the
   Clang database **match exactly** (a source must be built by both). A
   mismatch is a configuration error (exit status 2) listing which units are
   only in one database.

For each compiler cell the command becomes:

```text
<compiler> [entry args] [-std=c89|c23 + green baseline flags] <file>
```

`green` applies its own strict profile with `-fsyntax-only`; the original
build remains authoritative for producing binaries.

---

## Command reference

```
usage: green <command> [options]

commands:
  check [file...]      full seven-check matrix, lint, and format
  format [--check]     verify or rewrite canonical formatting
  lint [file...]       clang-tidy semantic checks (C89 and C23)
  matrix               four compiler cells only
  semantic <file.i>    semantic-only checks on preprocessed input
  fix                  apply safe, semantics-preserving fix-its
  doctor               validate the toolchain environment
  --version            print version and exit
```

Running `green` with no arguments, `--help`, or `-h` prints the usage text.
`--version` / `-V` prints the version and the LLVM/Clang build versions.

`doctor` and `--version`/`--help` run without a `green.yaml`. All other
commands require `green.yaml` and both compilation databases.

### `green check [file...]`

Run the full seven-check verification: toolchain capability, the GCC/Clang
matrix in C89 and C23, the clang-tidy semantic suite in both standards, and
format verification. Prints a final matrix (see
[Interpreting output](#interpreting-output-and-exit-status)).

With no file arguments it checks **every** `.c` unit in the compilation
databases. With file arguments it checks only those translation units;
headers they include still receive diagnostics when owned. File arguments are
used as given (the Clang database directory is passed to clang-tidy via `-p`
for include/define context).

### `green matrix`

Run only the four compiler cells (GCC C89, GCC C23, Clang C89, Clang C23).
Prints one `PASS`/`FAIL` line per cell over the whole database, followed by
captured compiler output for any failing cell.

### `green lint [file...]`

Run only the clang-tidy semantic/preprocessor checks, in both C89 and C23,
without the compiler matrix or format check. Operates over the whole database
by default, or over the named files. Each standard is run separately, so
diagnostics are reported per `(standard, file)`.

### `green format [--check]`

- `green format --check` verifies canonical formatting against the installed
  `clang-format.yaml` profile: it runs clang-format with `--dry-run --Werror`
  per file, without modifying source, and fails if any file is non-canonical.
  This is the reliable, supported verification form.
- `green format` (no `--check`) runs clang-format **without** `--dry-run` and
  without `-i`, so it emits the reformatted source to stdout and does not
  rewrite files in place. It is not a reliable verification form; use
  `green format --check` (or `green check`, which always uses the check form)
  to verify.

`green check` always uses the verify (`--check`) form. Format operates over
the whole database (every `.c` unit).

### `green fix`

Apply mechanically semantics-preserving fix-its to every database unit:

1. First normalize formatting (`clang-format -i` with the green profile) so
   braces/alignment are canonical before text edits.
2. Then run `clang-tidy --fix` restricted to the **safe, mechanical** checks:
   - `readability-braces-around-statements` (add missing braces),
   - `green-null` (spell `NULL`),
   - `green-transition-boundary` (discarded-result postfix → prefix, etc.).

Fix applies only mechanically safe transformations; it never rewrites
semantics it cannot prove. After `fix`, run `green check` to confirm the
result is fully green. (Redundant pointer-cast removal is anticipated but the
primary safe set in this release is braces, `NULL`, and postfix→prefix.)

### `green semantic <file.i>`

Run semantic-only checks on a preprocessed `.i` file. Preprocessed input
cannot receive normative checks that require the original source spelling
(for example `green-null`'s distinction of `NULL` vs `(void *)0`, or
ownership by real source path), so this is a best-effort semantic pass for
already-expanded input. It also requires a `<file.i>` argument; missing
argument is a configuration error (exit status 2).

### `green doctor`

Validate the toolchain environment:

- prints the version of `gcc`, `clang`, `clang-tidy`, `clang-format` (or
  `MISSING`),
- prints the plugin's LLVM/Clang build version,
- probes GCC for both `-std=c89` and `-std=c23` capability against the green
  baseline.

A missing tool, or a GCC that cannot drive C89/C23 under the green baseline,
prints `FATAL: ...` and exits with status 3. Otherwise exits 0.

---

## Interpreting output and exit status

### Exit status

| Code | Meaning |
| --- | --- |
| `0` | fully green |
| `1` | source/profile violation (`EXIT_VIOLATION`) |
| `2` | configuration or input error (`EXIT_CONFIG`) |
| `3` | required toolchain capability unavailable (`EXIT_UNAVAILABLE`) |
| `4` | internal `green` failure (reserved; `EXIT_INTERNAL`) |

`EXIT_VIOLATION` is produced when any check fails for a source reason.
`EXIT_UNAVAILABLE` is produced when a tool is missing, a compiler cannot take
a required option (detected by "unrecognized command-line option" /
"invalid option" in compiler output), a plugin fails to load, or `doctor`
finds a fatal capability gap. `EXIT_CONFIG` covers a missing/unparseable
`green.yaml`, a database that cannot be read or yields no `.c` files, a
GCC/Clang translation-unit set mismatch, an unknown command, or a `semantic`
command with no `.i` argument.

### `green check` output

On success it prints a result matrix:

```text
            C89    C23
GCC         PASS   PASS
Clang       PASS   PASS
clang-tidy  PASS   PASS

format      PASS

CGREEN      PASS
```

- The first four rows correspond to the four compiler cells.
- `clang-tidy` is a single aggregated verdict across both standards (green
  always runs lint under C89 and C23); the two columns carry the same value.
- `format` is a single column (format is not standard-moded).
- `CGREEN` is the conjunction of all four matrix cells, tidy, and format.

Clang-tidy and format diagnostics appear on stderr/stdout as they are
produced, before the summary table.

On any source failure the relevant cell prints `FAIL`; the affected
sub-check's detailed diagnostic or compiler output is shown above/below the
table, and the process exits 1.

### `green matrix` output

```text
GCC C89 PASS
GCC C23 PASS
Clang C89 PASS
Clang C23 PASS
```

A failing cell prints its captured compiler output beneath the `FAIL` line.
Exit status is 0 if all four pass, else 1.

### Reading clang-tidy diagnostics

A semantic diagnostic looks like:

```text
demo.c:13:9: error: inline computation inside a control/block body; extract it
                    into a worker function [green-flat]
```

- `demo.c:13:9` is `file:line:column`.
- The message explains the violation.
- `[check-name]` at the end names the `green-*` (or reused built-in) check
  that fired.

All `green-*` violations are reported at `error:` severity. They are emitted
independently for C89 and C23 runs, so you may see a violation twice (once per
standard) when a unit is checked under both.

### Toolchain / capability vs. source failures

If green cannot complete a check for an environment reason (missing tool,
unsupported compiler option, plugin load failure) rather than a source reason,
it reports `FATAL`/capability diagnostics and exits **3**, not 1. Run
`green doctor` to separate toolchain problems from source problems quickly.

---

## Worked example

The repository ships a sample project at `tests/driver/sample` with
`compile_commands.json` for both gcc and clang, a `green.yaml`, and
`src/demo.c`.

```sh
cd tests/driver/sample
# use the installed/built green driver
../../.agent/tmp/build/green doctor
../../.agent/tmp/build/green check
```

A green `demo.c` produces:

```text
            C89    C23
GCC         PASS   PASS
Clang       PASS   PASS
clang-tidy  PASS   PASS

format      PASS

CGREEN      PASS
```

and exits 0. Introduce a violation (for example change `if (p != NULL)` to
`if (p != 0)`, or add `s = s + i;` inside a loop body) and `green check` will
flag the owning check, print `FAIL`, and exit 1.

### To green a real project

1. Configure your build to emit a `compile_commands.json` (e.g. with CMake
   `CMAKE_EXPORT_COMPILE_COMMANDS=ON`) for **both** gcc and clang builds.
2. Create `green.yaml` pointing at both databases, with `project_roots`
   naming your sources and `exclude`/`compatibility_paths` as needed.
3. Run `green check`. Iterate: fix compiler-profile issues, semantic
   violations, then formatting.
4. Optionally run `green fix` to apply the safe mechanical fixes (braces,
   `NULL`, postfix→prefix) and re-check.
5. Gate CI on `green check` exiting 0.

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
examples). Each fixture declares its expectations in a metadata header
comment:

```c
/* green: lint=pass matrix=pass format=pass */
/* green: lint=green-hidden-control */
/* green: matrix=c23 */
/* green: format=fail */
```

A dimension is asserted only when tagged:

- `lint=pass` — no clang-tidy diagnostics under C89 and C23.
- `lint=<check>` — exactly that check fires under C89 and C23.
- `matrix=pass` — gcc+clang, C89+C23, all compile clean under the baseline.
- `matrix=c89|c23` — that standard's two cells fail; the other passes.
- `format=pass` / `format=fail` — canonical (or non-canonical) clang-format.

The dedicated `green-flat` catalog (`flat-*.c`) holds thin-body pass fixtures
and inline-work/over-glued-run fail fixtures across `if`/`else`, loops,
`switch`, and bare blocks.

Concept, acceptance criteria, design, stories, and BDD scenarios are kept
under `.agent/` (`.agent/concept`, `.agent/acceptance`, `.agent/stories`,
`.agent/design`, `.agent/testing`).

`just sanitize` runs only the lint-tagged fixtures under ASan/UBSan; matrix
and format fixtures are excluded because they do not fit the
zero-or-targeted-diagnostic model.

---

## Limitations

- The `green-tidy` plugin requires a Clang/`clang-tidy` ABI match (see
  [Requirements](#requirements)); `green doctor` reports a fatal mismatch.
- `green format` is verify-oriented in this release; `green format --check`
  is the reliable form. `green fix` applies only the mechanically safe subset
  of fix-its.
- `green semantic` cannot apply normative source-spelling checks to
  preprocessed input.
- A `switch` case body is flat-scanned only when braced.
- On toolchains where valgrind cannot load the monolithic `libclang-cpp.so`,
  `just valgrind` reports `UNSUPPORTED`; memory-safety/UB coverage is provided
  by `just sanitize` (ASan + UBSan).
- V1 has no inline `NOLINT` escape.
