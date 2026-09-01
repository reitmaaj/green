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
green-cast-boundary         numeric/void-pointer/redundant/representation cast policy
green-null                  NULL spelling, source-vs-expansion
green-declaration           one declarator per declaration; prototypes; (void)
green-fallthrough           implicit rejected; exact /* fall through */ marker
green-preprocessor          function-like macros, pasting/stringify, macro-generated structure
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

## Compatibility boundary

Files under `compatibility_paths` still pass the four compiler cells and
format, but MAY receive enumerated exemptions from the compiler-identity
preprocessing ban, the representation-cast ban, and other designated
portability-boundary rules. Hidden control, hidden mutation, mandatory
braces, and warning cleanliness remain mandatory.
