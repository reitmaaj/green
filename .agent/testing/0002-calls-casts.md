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

SCENARIO redundant void-pointer cast is rejected
GIVEN `node = (struct node *)raw;` where implicit `void *` conversion is
      well-typed
WHEN `green lint` runs
THEN a `green-cast-boundary` error is reported

SCENARIO representation escape is rejected
GIVEN integer<->pointer, unrelated-object-punning, object<->function, or
      qualifier-discarding casts
WHEN `green lint` runs
THEN a `green-cast-boundary` error is reported
