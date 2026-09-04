# Testing: calls, purity, and casts

SCENARIO effect call must form a complete transition
GIVEN `n = read_record(fd, buf, size) + 1;` or `consume(f());` or
      `return f();` or `if (f()) { }`
WHEN `green lint` runs
THEN a `green-effect-boundary` error is reported

SCENARIO effect call with result binding is accepted
GIVEN `n = read_record(fd, buf, size);` as a complete statement
WHEN `green lint` runs
THEN no error is reported

SCENARIO effect result binding survives a transparent wrapper
GIVEN `struct node *p; p = allocate();` where `allocate` returns `void *`
      (Clang inserts an implicit `void * -> struct node *` conversion), or
      `p = (struct node *)allocate();`
WHEN `green lint` runs
THEN no `green-effect-boundary` error is reported; the wrapper is
     placement-transparent, and cast admissibility is left to
     `green-cast-boundary`

SCENARIO consuming a transparent effect result is still rejected
GIVEN `x = -produce();` or `x = *produce();` or `x = produce()[0];` or
      `x = (int)produce() + 0;`
WHEN `green lint` runs
THEN a `green-effect-boundary` error is reported (a wrapper is transparent
     only for placement; computing from or consuming the result is not)

SCENARIO effect placement does not approve an escaping cast
GIVEN `p = (struct foo *)(uintptr_t)allocate(n);` where the cast chain
      contains an integer/pointer escape
WHEN `green lint` runs
THEN `green-effect-boundary` reports nothing (the effect is properly
     sequenced); `green-cast-boundary` reports the bad cast

SCENARIO an indirect call is conservatively effectful
GIVEN a call through a function pointer, `x = (*fp)(arg);` or `(*fp)(arg);`
      as a complete statement
WHEN `green lint` runs
THEN no `green-effect-boundary` error is reported

SCENARIO an indirect effect cannot hide inside an expression
GIVEN `x = (*fp)(arg) + 1;` or `if ((*fp)(arg)) { }` or `return (*fp)(arg);`
WHEN `green lint` runs
THEN a `green-effect-boundary` error is reported

SCENARIO the complete-transition test is measured on the outermost expression
GIVEN a discarded effectful call whose whole statement is wrapped in
      parentheses, `(f());`
WHEN `green lint` runs
THEN no `green-effect-boundary` error is reported; a transparently-wrapped
     call that forms an entire statement is still a complete transition

SCENARIO a parenthesized effect is still rejected when buried in an expression
GIVEN `x = (f()) + 1;` (parentheses around the call do not make it a complete
      transition when it feeds an arithmetic result)
WHEN `green lint` runs
THEN a `green-effect-boundary` error is reported

SCENARIO configured purity reaches the effect check
GIVEN a function name listed in the `pure_functions` config list
WHEN `green lint` runs through the driver on that configuration
THEN calls to it are treated as pure (usable in expression position)

SCENARIO pure call belongs to the term language
GIVEN `x = pure_abs(a) + pure_abs(b);` and `pure_abs` is marked `GREEN_PURE`
WHEN `green lint` runs
THEN no error is reported

SCENARIO false purity is rejected
GIVEN a `GREEN_PURE`-marked function whose body mutates or calls an effect
WHEN `green lint` runs
THEN a `green-pure-contract` error is reported

SCENARIO pure marker placement
GIVEN `GREEN_PURE` not on its own token line or not immediately before a
      declaration/definition
WHEN `green lint` runs
THEN a `green-pure-contract` error is reported

SCENARIO intentional numeric cast is accepted
GIVEN a guarded narrowing or signedness cast such as `(unsigned int)size`
WHEN `green lint` runs
THEN no error is reported

SCENARIO same-type cast is redundant
GIVEN a cast from a type to its same canonical type
WHEN `green lint` runs
THEN a `green-cast-boundary` error is reported

SCENARIO void-pointer conversion is accepted in both directions
GIVEN `node = (struct node *)raw;`, `return (struct node *)raw;`, or
      `take((void *)node);` where implicit `void *` conversion is well-typed
WHEN `green lint` runs
THEN no error is reported (whether the cast is written explicitly or implied)

SCENARIO unrelated object-pointer conversion is accepted
GIVEN `pb = (struct b *)pa;` where `struct a *` and `struct b *` are distinct
      object pointer types
WHEN `green lint` runs
THEN no error is reported; access through the converted pointer remains
     subject to C's effective-type/aliasing rules

SCENARIO representation escape is rejected
GIVEN integer<->pointer, object<->function, or qualifier-discarding casts
WHEN `green lint` runs
THEN a `green-cast-boundary` error is reported
