# Acceptance: informative diagnostics (LLM-consumable findings)

Findings are consumed by automated fixers, so each diagnostic must be
self-contained: it must state the rule, the offending construct, and the
profile-canonical remedy without requiring the rule documentation.

## Every green finding

MUST begin with the terse violation summary that names the check's concern.
MUST be a single line: no newline or carriage return inside the message, so
clang-tidy's `[check-name]` suffix and source/caret rendering stay intact.
MUST carry a WHY section stating the principle the check enforces.
MUST carry a CONTEXT section naming the concrete offending construct (the
operator, statement kind, function, macro, or types involved) whenever the
finding has dynamic facts to report.
MUST carry a FIX section prescribing the accepted rewrite.
MUST NOT prescribe a fix that violates the profile itself (no postfix
`++`/`--`, no `&&`/`||`/`?:`/`,` operators in fix examples, no unbraced
bodies, no `0` for a null pointer, no `_t` names, no multi-object
declarations).
MUST NOT be produced for a location the project does not own.
MUST NOT change which check fires or the finding count per construct: only
the message text grows richer.

## Per-check guidance content

- `green-hidden-control`: FIX names the explicit braced rewrite per operator
  (`&&` -> nested `if`s; `||` -> negated `else if` chain or branch restructure;
  `?:` -> if/else that assigns the result; `,` -> separate statements) and
  distinguishes the comma operator from the separator comma.
- `green-transition-boundary`: FIX states transitions are standalone
  statements or single `for` clauses; only prefix `++`/`--`.
- `green-effect-boundary`: CONTEXT names the callee (or "indirect call") and
  the consuming placement; FIX lists standalone discard, sole-RHS result
  binding, `for` init/inc, and the pure-function route
  (`GREEN_PURE`/`pure_functions`) for genuine purity.
- `green-pure-contract`: CONTEXT names the function and the offending body
  statements; FIX says to drop the annotation or make the body truly pure.
- `green-cast-boundary`: CONTEXT quotes source and destination types; FIX says
  to remove the redundant cast / restore qualification / avoid the
  representation escape.
- `green-null`: FIX prescribes `NULL` (via `<stddef.h>`).
- `green-reserved-suffix`: CONTEXT names the type and kind (alias or tag);
  FIX prescribes a name not ending in `_t`.
- `green-declaration`: FIX prescribes one object per declaration; prototype
  forms with `(void)`.
- `green-fallthrough`: FIX names `break;` or the exact `/* fall through */`
  marker as the last statement of the case.
- `green-preprocessor`: CONTEXT names the macro; FIX prescribes a function /
  object-like constant / explicit statements.
- `green-toolchain-branching`: FIX states the intersection requirement and
  names `compatibility_paths` for narrow shims.
- `green-flat`: FIX prescribes extracting inline computation into a worker
  function the thin block calls.
- `green-braces`: CONTEXT names the controlled statement kind; FIX prescribes
  `{}` wrapping, `{}` for empty bodies.

## Driver surfaces

MUST NOT emit a silent FAIL: a failing matrix cell under `green matrix` or
`green check` MUST print the cell identity, the enforced mode/baseline flags,
and the compiler's diagnostics; a failing format check MUST print a violation
line naming the file plus the clang-format diagnostics that localize each
non-canonical line.
