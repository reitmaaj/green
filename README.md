# green

A strict C89 ∩ C23 source-profile toolchain: one project source that
compiles warning-clean as strict C89 and strict C23, under both GCC and
Clang, with explicit semantic structure and one canonical Allman format.

See `.agent/` for concept, stories, acceptance, design, and testing.

Build and test with `just`:

- `just setup` — configure the CMake build
- `just build` — build the driver and the `green-tidy` plugin
- `just unit` — run unit tests
- `just e2e` — run the fixture suite
- `just test` — unit + e2e
- `just lint` — shellcheck + clang-format dry-run
- `just check` — test + lint
- `just sanitize` — build `green`/`green-tidy` with AddressSanitizer + UBSan
  and run the full fixture suite under them (memory-safety/UB verification)
- `just valgrind` — attempt the driver + plugin under valgrind memcheck;
  degrades gracefully (UNSUPPORTED) on toolchains where valgrind cannot load
  the monolithic libclang-cpp.so
