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
