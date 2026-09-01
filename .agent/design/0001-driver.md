# Driver design

## Components

```text
src/driver/main.cc        CLI entry, subcommand dispatch
src/driver/Config.cc      green.yaml parsing and validation
src/driver/CompileDB.cc   compilation-database normalization
src/driver/Matrix.cc      four-cell compiler matrix execution
src/driver/Process.cc     subprocess execution
src/driver/Format.cc      clang-format orchestration
src/driver/Fix.cc         safe fix-it application
src/driver/Doctor.cc      environment/toolchain validation
```

## Configuration (`green.yaml`)

```yaml
version: 1
compile_commands:
    gcc: build/gcc/compile_commands.json
    clang: build/clang/compile_commands.json
project_roots: [src, include]
exclude: [build/**, vendor/**]
compatibility_paths: [src/compat/**]
pure_functions: []
```

- Unknown keys are errors.
- Relative paths resolve from the directory containing `green.yaml`.
- Configuration inheritance is not supported in V1.

## Compilation-database normalization

From each command preserve source-semantic build information:
`-I -isystem -D -U -include`, target/ABI selection, language include paths,
project feature definitions.

Remove or supersede: `-std=... -W... -o ... -c`, dependency-file generation,
link-only options, code-generation-only options. Then apply the canonical
`green` standard and warning profile.

The normalized set of project `.c` translation units in both databases MUST
match. A mismatch is a configuration failure (exit 2). `green` performs
source-profile checking with `-fsyntax-only`; the original build remains
authoritative for producing binaries.

## `doctor`

Reports GCC path/version, Clang path/version, clang-tidy path/version,
clang-format path/version, plugin LLVM/Clang build version, C89/C23
capability, required warning-option capability, compilation-database
status, and format-profile compatibility. A clang-tidy/plugin ABI mismatch
is fatal.

## Version matching

The driver and plugin record the LLVM/Clang version at build time. `green
doctor` selects a plugin only when its recorded Clang version matches the
selected `clang-tidy` installation, because Clang provides no ABI/API
stability guarantee for out-of-tree plugins.
