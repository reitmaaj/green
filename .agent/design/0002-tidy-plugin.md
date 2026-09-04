# clang-tidy plugin design

`green-tidy` is a clang-tidy plugin built as a loadable `MODULE`. Clang
explicitly supports out-of-tree checks using AST Matchers and
`PPCallbacks`, loaded with `clang-tidy -load`. Because Clang provides no
ABI/API stability guarantee, the plugin MUST match the clang-tidy version
against which it was built.

## Checks

```text
green-hidden-control        && || ?: comma operator
green-transition-boundary   mutation only as complete statement/for-clause; prefix-only ++/--
green-effect-boundary       effect call must form a complete transition
green-pure-contract         GREEN_PURE marker + pure_functions + false-annotation lint
green-cast-boundary         numeric/qualifier/representation cast policy (object-pointer
                            conversions allowed)
green-null                  NULL spelling, source-vs-expansion
green-declaration           one declarator per declaration; prototypes; (void)
green-reserved-suffix       no owned type name ending in the reserved _t suffix
green-fallthrough           implicit rejected; exact /* fall through */ marker
green-preprocessor          function-like macros, pasting/stringify, macro-generated
                            control/transition via expansion-site attribution
green-toolchain-branching   __STDC_VERSION__/__GNUC__/__clang__ conditionals
```

Reused built-in: `readability-braces-around-statements` with
`ShortStatementLines = 0`.

## Principles

- AST checks use semantic nodes, not textual regexes.
- Preprocessor rules use `PPCallbacks`.
- Source spelling and macro provenance use `SourceManager` (e.g. `NULL` vs
  `(void *)0`).
- All `green-*` checks report only project-owned source unless configuration
  explicitly extends their scope.
- V1 provides no inline `NOLINT` escape for `green-*` checks. Exemptions are
  localized by `compatibility_paths` file boundaries.

## Ownership

Ownership is path-based and independent of compiler classification:

```text
if project_roots is nonempty:
    owned(file) := beneath(canonical(file), any project_root)
else:
    owned(file) := beneath(canonical(file), canonical(dirname(main TU)))
owned(file) := owned(file) && !beneath(canonical(file), any exclude)
```

- `beneath` compares path components, not string prefixes, so
  `/project/src` owns `/project/src/x.c` but not `/project/src-old/x.c`.
- Paths are canonicalized via `real_path`; canonical roots/excludes are
  computed once when options are loaded, not per AST node.
- A source location that cannot resolve to a real source file (virtual,
  builtin, or scratch buffers) is simply not owned.
- `-isystem`, system-header bits, and angle-vs-quoted include status MUST
  NOT affect ownership.

## Object-like macro policy (expansion-site attribution)

green rejects function-like macros and token-pasting/stringification macros
at definition time. For object-like macros green inspects the semantic AST
produced by expansion rather than the replacement-list punctuation:

- Each of `green-hidden-control`, `green-transition-boundary`, and
  `green-preprocessor` keeps a per-check/TU `MacroRegistry` populated by its
  own `PPCallbacks`, recording project-owned object-like macros by
  definition source range. No module-global or static state.
- `findOwnedGoverningMacro(loc)` walks the macro-expansion ancestry via
  immediate expansion/caller locations (not a single spelling lookup) and
  returns the innermost owned object-like macro responsible.
- When a control/transition construct's governing token originates from an
  owned object-like macro, `green-preprocessor` reports it attributed to
  that macro; `green-hidden-control` and `green-transition-boundary` defer
  so the diagnostic is attributed once, to the macro.
- Pure value/type/token object-like macros (arithmetic, bitwise,
  `sizeof(struct {...})`, `void *` constants) are accepted because their
  expansions introduce no control or transition.

## Compatibility boundary

Files under `compatibility_paths` still pass the four compiler cells and
format, but MAY receive enumerated exemptions from the compiler-identity
preprocessing ban, the representation-cast ban, and other designated
portability-boundary rules. Hidden control, hidden mutation, mandatory
braces, and warning cleanliness remain mandatory.
