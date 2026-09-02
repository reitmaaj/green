# Design: outline

## Module boundaries

The transform core is the vendored bb-lifter (`clang-outliner`) subtree, kept
under `src/outline/bblift` with its upstream file layout, formatting, and
`bblift` namespace. It is a separate compile unit so its (llvm) formatting is
not forced into green's profile.

```text
src/outline/bblift/        vendored bb-lifter core (frontend/analysis/model/
                           emit/verify), namespace bblift
    src/outline/bblift/main.cc   its CLI (built as the `green-outline` binary)
src/driver/Outline.h/.cc   green-side entry point (mirrors runFix/runLint)
```

`green outline` is a driver command that shells out to the `green-outline`
binary with the project's clang compilation database, exactly as green already
shells out to `clang-tidy`/`gcc`/`clang`/`clang-format`. It is not linked into
the driver's process; this keeps bb-lifter's `llvm::cl`/`CommonOptionsParser`
CLI out of green's own argument parsing.

`green outline` per file:
1. skip the file when the green-outline gate finds no un-extracted blocks
   (idempotence);
2. run `green-outline` to transform the file in place;
3. run the canonical formatter;
4. re-run the full clang-tidy suite in C89 and C23; any diagnostic fails the
   command (fail-safe).

## CFG authority

- Only `CFGBuilder` constructs `clang::CFG`.
- Only `CFGNormalizer` reads `clang::CFG` internals.
- Pinned build options mirror the accepted compatibility surface used for
  control-flow analysis: `AddInitializers = true`; pruning of trivially
  false edges disabled; EH/temp/lifetime/scope edges disabled.
- The CFG entry/exit pseudo-blocks are normalized away; every source
  `return` transitions to a synthetic `DONE` state; a `void` function
  falling off the end also transitions to `DONE`.

## Block definition

An emitted block is a maximal single-entry source-emittable region derived
from one or more `clang::CFGBlock`s that can be emitted without splitting
an indivisible C full expression, ending in one explicit normalized control
transfer. Collapse a block with exactly one unconditional successor, no
externally targetable source label, and no semantic element; never collapse
branch points, switch dispatch nodes, or direct goto targets.

## Green-conformant emitters

Hidden control in expression position is forbidden by
`green-hidden-control`. The outliner therefore never emits `cond ? A : B`
as a terminator. This is the one deliberate delta from upstream bb-lifter:
vendored `src/outline/bblift/emit/HelperEmitter.cc` lowers the `Branch`
terminator to the braced else-less form below instead of `?:`. Transfers:

- Fallthrough: `return B_NEXT;`
- Branch: an explicit `if (cond) { return B_TRUE; }` followed by
  `return B_FALSE;` (never a bare single statement; Allman braces
  mandatory).
- Switch: a real `switch` whose `case` bodies each `return` their target
  state.
- Goto: direct goto resolved through `GotoStmt -> LabelDecl -> containing
  block`, emitted as `return B_LABEL;`. Computed goto and address-of-label
  are rejected.
- Return: non-void emits `s->result = EXPR;` then `return DONE;`; void
  emits `return DONE;`.
- `break`/`continue` are resolved through CFG topology and structured AST
  parentage during normalization; never emitted as statements.

Every controlled body uses braces (`readability-braces-around-statements`,
`ShortStatementLines = 0`), braces are Allman, and output round-trips the
canonical format profile (`format=pass`). Emitted assignments and updates
form complete transitions only, so `green-transition-boundary` passes.

## Dispatcher

The original function is reduced to: declare the frame and program counter,
initialise parameters into the frame, set `pc` to the entry state, then

```c
for (;;)
{
    switch (pc)
    {
    case F_X:
        pc = f_x(&s);
        break;
    case F_DONE:
        return <result>;
    }
}
```

`F_DONE` is handled inside the dispatcher switch; it never becomes a
helper. The top-level function's name and signature are preserved so callers
are unaffected.

## Naming

Generated identifiers (`f_pc`, `f_frame`, `f_bN`, `pc`, `s`, `F_B0`,
`F_DONE`) are chosen to avoid collisions with existing identifiers in the
translation unit (NameGenerator). Field and state names never depend on
token spelling of source identifiers.

## Eligibility and rejection

Outlining never approximates. A function that cannot be outlined cleanly is
reported with an explicit reason and left unchanged. Rejected constructs
include VLA, block-scope static, asm, statement expressions, computed goto
and address-of-label, setjmp/longjmp, macro-boundary rewrites, and a body
that cannot be partitioned into single-entry source-emittable blocks
(including functions whose source is owned by a project object-like macro).
`main` is left untouched.

## Idempotence (recognition)

A function passes the gate iff re-outlining it is a no-op. Outlined output
is exactly the canonical dispatcher (single `for (;;) { switch (pc) {...} }`
whose cases call single-block helpers taking one frame pointer and returning
the pc enum). Any owned function whose body still contains ordinary,
un-extracted internal control flow (more than one basic block that is not
the canonical dispatcher) is flagged by `green-outline`.

## Whole-function atomicity

Eligibility is decided for every candidate in the translation unit before
any output. If any owned function is not outlined cleanly, `green` reports
the failure and writes nothing for that unit (no partial file modification).
