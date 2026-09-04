# Acceptance: formatting and configuration

## green-format profile

- Core style: 4-space indent, no tabs, Allman braces, right pointer
  alignment, no short blocks/functions/if/loops on a single line, 80-column
  limit, no case/goto label indentation.
- `green format` writes canonical formatting; `green format --check` verifies
  without modifying.
- The installed profile is authoritative even if a project also carries a
  `.clang-format` copy for editor integration.

## Configuration (`green.yaml`)

- `version: 1` is required.
- `compile_commands` is a mapping of `gcc`/`clang` sub-keys; `project_roots`,
  `exclude`, `compatibility_paths`, and `pure_functions` are block lists. Each
  block list MUST populate from its own top-level key and the `- item` lines
  that follow it.
- `compile_commands.gcc` and `compile_commands.clang` point to JSON
  compilation databases.
- Unknown configuration keys are errors.
- Relative paths resolve from the directory containing `green.yaml`.
- `compatibility_paths` declare narrow exemption boundaries.
- `pure_functions` lists external pure functions and MUST reach the
  `green-effect-boundary`/`green-pure-contract` checks' options via the driver.

## Unacceptable behaviors

- A `green.yaml` with unknown keys MUST be rejected (exit 2).
- Missing `version: 1` MUST be rejected.
- A header MUST NOT receive a standalone invented interpretation.
- A `- item` list entry that appears with no active block-list header (an
  orphaned list item) MUST be rejected, not silently ignored.
- Block-list parsing MUST NOT be broken by the preceding top-level empty-value
  header (e.g. an `exclude:` header must not reset the parser so that its
  `- item` lines are dropped).
