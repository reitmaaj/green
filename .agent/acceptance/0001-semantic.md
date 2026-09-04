# Acceptance: semantic checks

Each check has a minimal passing fixture, a minimal failing fixture, edge
cases, macro-origin fixtures where relevant, and both C89 and C23 tidy
execution.

## green-hidden-control

- PASS: arithmetic/comparison/bitwise/logical-not expressions, nested pure
  computation.
- FAIL: `&&`, `||`, `?:`, and the comma operator in expression position.
- FAIL: `if (a && b) { ... }`; `x = cond ? a : b;`.
- Note: the comma separating arguments, declarators, initializer elements,
  or `for` clauses is not the comma operator and is accepted.

## green-transition-boundary

- PASS: `x = y; x += n; ++i; --i;` as complete statements; `for (i=0;i<n;++i)`.
- FAIL: `x = ++i; f(i++); x = (y = z); x += (y += z); a[i++] = value;`.
- FAIL: postfix `++`/`--` in any position.
- FAIL: effectful initializer `int fd = open_file(path);`.
- FAIL: a `for` clause containing more than one transition.

## green-effect-boundary

- PASS: effect call alone as a statement; effect call as the sole right-hand
  side of an assignment (result binding); an effectful call as a `for`
  increment clause (`for (...; ...; step())`); an explicit `(void)f();`
  discarded statement.
- PASS: the bound effect result may pass through parentheses or any implicit
  or explicit cast wrapper, e.g. `p = allocate();` where `allocate` returns
  `void *` (Clang inserts an implicit `void * -> struct node *`), or
  `p = (struct node *)allocate();`. The wrapper is placement-transparent only:
  `green-effect-boundary` reports nothing and cast admissibility is judged
  solely by `green-cast-boundary`.
- FAIL: `n = f() + 1; consume(f()); if (f()) {} return f();`; an effectful
  call in a `while`/`do`/`for` *condition*.
- FAIL: consuming or computing from a transparently-wrapped effect result
  (`x = -f();`, `x = *f();`, `x = f()[0];`, `x = (int)f() + 0;`,
  `x = (f()) + 1;`).
- PASS: a transparently-wrapped effect that forms an *entire statement*
  (`(f());`) is a complete discarded statement; the complete-transition test is
  measured on the outermost expression.
- FAIL: an indirect call (through a function pointer) is conservatively
  effectful, so `x = (*fp)(a) + 1; if ((*fp)(a)) {} return (*fp)(a);`
  fail while `(*fp)(a);` and `x = (*fp)(a);` pass.
- PASS: a function listed in the `pure_functions` config is treated as pure
  when the driver serializes the list into the check's options.
- PASS: a `GREEN_PURE`-marked *prototype* asserts purity of an external
  function, so calls to it are usable in expression position (unvalidated
  trust, like `pure_functions`).

## green-pure-contract

- PASS: a `GREEN_PURE`-marked function whose body is pure.
- FAIL: a marked function whose body contains mutation, an effect call, a
  volatile access, or another unproven effect.
- FAIL: `GREEN_PURE` not on its own token line or not immediately preceding
  a declaration/definition.
- FAIL: a false PURE annotation.

## green-cast-boundary

- PASS: intentional narrowing/signedness/domain casts (e.g. guarded
  `(unsigned int)size`, `(unsigned char)value`).
- FAIL: a cast from a type to its same canonical type.
- PASS: `void *` conversions in both directions, written or implied
  (`(struct node *)raw`, `(void *)node`).
- PASS: unrelated object-pointer conversions (`pb = (struct b *)pa;`).
  Access through the converted pointer is governed by C's
  effective-type/aliasing rules, not by this cast check.
- FAIL: integer<->pointer casts, object<->function pointer casts, and casts
  discarding const/volatile.

## green-null

- PASS: `p = NULL;` and `if (p == NULL)`.
- FAIL (stylistic): `p = 0; p = (void *)0; p = (struct node *)0;`.
- PASS: `(void *)0` that arises from the expansion of the implementation's
  `NULL` macro (source-spelling distinction via SourceManager).

## green-declaration

- PASS: one object per declaration; prototypes; `int poll_status(void);`.
- FAIL: `int count, index; char *name, buffer[32];` for locals, file scope,
  and struct/union fields.
- FAIL: K&R definitions and unspecified-parameter declarations.
- Note: the rule does not apply to function parameter lists.

## green-fallthrough

- PASS: `break` terminated cases; intentional `/* fall through */` marker
  after the last statement before the next case/default.
- FAIL: implicit fallthrough without the marker.
- FAIL: any marker other than the exact canonical spelling.

## green-preprocessor

- PASS: `#include`, object-like macros, ordinary include guards.
- FAIL: project-defined function-like macros.
- FAIL: token-pasting and stringification macros.
- FAIL: project object-like macros whose expansion injects hidden control or
  transition structure (e.g. `? :`, `&&`, `||`, assignment, `return`,
  statement blocks around computation). Attribution walks the macro
  expansion ancestry and reports the innermost owned object-like macro.
- PASS: project object-like macros whose expansion is a pure value/type/
  token abstraction (arithmetic, bitwise, `sizeof(struct {...})`, `void *`
  constants). The empty `GREEN_PURE` annotation is exempt.
- Applies only to project-owned files; ownership is determined by
  `project_roots` (or, when unconfigured, the primary translation unit's
  directory tree), never by `-isystem`/system-header classification.

## Ownership

- A source location is project-owned when its canonical file path lies
  beneath a configured `project_roots` entry (or, when none are configured,
  beneath the canonical directory of the primary translation unit) and is
  not beneath an `exclude` entry.
- Containment compares path components, not string prefixes: `/project/src`
  owns `/project/src/x.c` but not `/project/src-old/x.c`.
- Compiler classification (`-isystem`, system-header bits, angle vs quoted
  include) MUST NOT affect ownership.
- Source locations that cannot resolve to a real source file (virtual,
  builtin, scratch buffers) are simply not owned.

## green-toolchain-branching

- FAIL: project source selecting implementations based on
  `__STDC_VERSION__`, `__GNUC__`, `__clang__`, or other compiler/version
  identity macros, outside configured compatibility paths.
- PASS: such tests inside `compatibility_paths`; system/third-party headers
  are exempt.

## Reused built-in: readability-braces-around-statements

- Every controlled body (`if/else/while/do/for`) uses braces; empty bodies
  use empty blocks, never null statements; `else if` remains a chain.

## green-flat

Rationale: structural decomposition / test-boundary discipline. Nested control
blocks orchestrate (thin delegation); worker functions compute. A loop body
that reduces to a single delegation (`hm_i_collect_free(...)`) is a successful
Green transformation.

- PASS: the function's own top level computes freely; a straight-line pure
  worker with no nested block is flat by construction.
- PASS: a nested control/`{}` body that is thin: no inline computation and at
  most one glue statement per straight-line run (a single discarded call, a
  single result-binding call, a single prefix `++`/`--`, or a single
  literal/call initializer, followed only by a decision/transfer).
- PASS: decision/iteration sites that delegate real work to a worker function
  (worker/controller rule).
- FAIL: any operator-built value inside a nested body: an assignment whose
  right-hand side is computed (`s = s + i;`), a compound assignment
  (`s += n;`), or a declaration initializer that computes (`int y = n * i;`),
  in an `if`/`else` branch, a loop body, or a bare `{}` block.
- FAIL: a straight-line run of more than one glue statement inside a nested
  body (two discarded calls, call plus prefix update, two bindings).
- Note: a `switch` case body is inspected only when the case forms its own
  nested `{}` block; unbraced case statements are not inline-scanned by the
  current check (see `flat-008`, `flat-017`, `flat-020`).
