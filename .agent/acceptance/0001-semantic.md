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
  side of an assignment (result binding).
- FAIL: `n = f() + 1; consume(f()); if (f()) {} return f();`.

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
- FAIL: a redundant `(struct node *)` cast where implicit `void *`
  conversion is well-typed with the same semantics.
- FAIL: integer<->pointer casts, unrelated object-pointer casts for type
  punning, object<->function pointer casts, and casts discarding
  const/volatile.

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
- FAIL: object-like macros that introduce runtime control or mutation
  syntax. The empty `GREEN_PURE` annotation is exempt.
- Applies only to project-owned files, never system/vendor headers unless
  explicitly requested.

## green-toolchain-branching

- FAIL: project source selecting implementations based on
  `__STDC_VERSION__`, `__GNUC__`, `__clang__`, or other compiler/version
  identity macros, outside configured compatibility paths.
- PASS: such tests inside `compatibility_paths`; system/third-party headers
  are exempt.

## Reused built-in: readability-braces-around-statements

- Every controlled body (`if/else/while/do/for`) uses braces; empty bodies
  use empty blocks, never null statements; `else if` remains a chain.
