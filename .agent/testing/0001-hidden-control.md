# Testing: hidden control and transitions

SCENARIO logical short-circuit is rejected
GIVEN an expression using `&&` or `||` in an `if` condition
WHEN `green lint` runs
THEN a `green-hidden-control` error is reported

SCENARIO ternary is rejected
GIVEN `x = cond ? a : b;`
WHEN `green lint` runs
THEN a `green-hidden-control` error is reported

SCENARIO comma operator is rejected
GIVEN the comma operator in an expression (not a separator)
WHEN `green lint` runs
THEN a `green-hidden-control` error is reported

SCENARIO mutation must be a complete transition
GIVEN `x = ++i;` or `f(i++);` or `x = (y = z);` or `a[i++] = value;`
WHEN `green lint` runs
THEN a `green-transition-boundary` error is reported

SCENARIO postfix update is rejected
GIVEN any postfix `++` or `--`, even with a discarded result
WHEN `green lint` runs
THEN a `green-transition-boundary` error is reported

SCENARIO prefix update is accepted
GIVEN `++i;` or `--i;` as a complete statement
WHEN `green lint` runs
THEN no error is reported

SCENARIO for-clause categories
GIVEN `for (i = 0; i < n; ++i) { }`
WHEN `green lint` runs
THEN no error is reported

SCENARIO for-clause rejects hidden control and multi-transition
GIVEN `for (i = 0; i < n && ready; ++i)` or `for (i = 0, j = 0; ...)`
WHEN `green lint` runs
THEN an error is reported
