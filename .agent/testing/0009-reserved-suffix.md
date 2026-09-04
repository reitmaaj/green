# Testing: green-reserved-suffix (authored type names)

The C standard reserves identifiers beginning with an underscore for the
implementation, and POSIX additionally reserves the `_t` suffix for the
implementation's own types. A project that authors its own type whose name
ends in `_t` risks colliding with a current or future system type once the
two translation units are combined. `green-reserved-suffix` therefore rejects
the `_t` suffix on type names the project itself **defines** (typedef names
and struct/union/enum tags). It never flags a type merely because it is
*used*: a system/library type such as `size_t`, `time_t`, or `int32_t` is
declared in a header that the project does not own, so its definition is not
reported.

Only owned definitions are judged. Definitions in files the project does not
own (system headers, `<lib.h>` libraries, vendored headers listed under
`exclude`) are always allowed. Owned files listed under `compatibility_paths`
are exempt so an interop shim may mirror a platform's exact spelling.

SCENARIO an owned typedef name ending in _t is reported
GIVEN a project file that authors `typedef struct node node_t;`
WHEN `green lint` runs
THEN a `green-reserved-suffix` error is reported at the typedef name

SCENARIO an owned scalar typedef ending in _t is reported
GIVEN a project file that authors `typedef unsigned long count_t;`
WHEN `green lint` runs
THEN a `green-reserved-suffix` error is reported

SCENARIO an owned struct tag ending in _t is reported
GIVEN a project file that defines `struct point_t { ... };`
WHEN `green lint` runs
THEN a `green-reserved-suffix` error is reported at the tag

SCENARIO an owned union tag ending in _t is reported
GIVEN a project file that defines `union value_t { ... };`
WHEN `green lint` runs
THEN a `green-reserved-suffix` error is reported at the tag

SCENARIO an owned enum tag ending in _t is reported
GIVEN a project file that defines `enum color_t { ... };`
WHEN `green lint` runs
THEN a `green-reserved-suffix` error is reported at the tag

SCENARIO a typedef wrapping a reserved tag is reported once
GIVEN a project file that authors `typedef struct node_t { ... } node_t;`
WHEN `green lint` runs
THEN exactly one `green-reserved-suffix` error is reported for that name

SCENARIO a system type used in owned code is accepted
GIVEN an owned file that uses `size_t` and `time_t` from system headers
WHEN `green lint` runs
THEN no `green-reserved-suffix` error is reported

SCENARIO an authored type without the suffix is accepted
GIVEN an owned file that authors `Node`, `struct point`, and `enum color`
WHEN `green lint` runs
THEN no `green-reserved-suffix` error is reported

SCENARIO a reserved suffix under compatibility_paths is exempt
GIVEN an owned file under `compatibility_paths` that authors a `_t` type
WHEN `green lint` runs
THEN no `green-reserved-suffix` error is reported
