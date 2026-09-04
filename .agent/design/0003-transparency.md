# Transparency and structural decomposition

## Two kinds of explicitness

Green enforces explicitness at two independent axes.

```text
expression transparency
    effects, mutation, sequencing, and value-dependent control
    cannot hide inside expressions
    (green-hidden-control, green-transition-boundary,
     green-effect-boundary)

structural transparency
    computation below a control-flow boundary acquires a named
    worker-function boundary
    (green-flat)
```

This makes `green-flat` a decomposition rule, not an anomalous complexity
rule: nested control regions orchestrate while worker functions compute.

## Value categories

```text
TERM              ordinary value computation

TRANSITION        explicit standalone mutation

EFFECT            explicit call/binding boundary

CONTROL           explicit braced control structure

WORKER            named unit containing substantive computation

CONTROLLER REGION nested block containing only thin orchestration,
                  at most one glue operation per straight-line run
```

## Effect placement vs cast approval

`green-effect-boundary` answers only *where an effect occurs relative to
sequencing and computation*. Cast admissibility is owned exclusively by
`green-cast-boundary`.

```text
effect_result :=
    CallExpr
    ParenExpr(effect_result)
    ImplicitCastExpr(effect_result)
    CStyleCastExpr(effect_result)
```

Placement analysis operates on the outermost transparent wrapper. Thus

```c
p = (struct foo *)allocate(n);
```

passes `green-effect-boundary` (the cast is placement-transparent) and the
`void * -> struct foo *` conversion is judged only by `green-cast-boundary`.
A cast chain containing a representation escape (e.g. through `uintptr_t`)
never produces an effect-placement diagnostic; only the bad cast is reported.

Transparency does NOT extend to value/unary/consuming operations:

```c
x = -produce();      /* effect-boundary error */
x = *produce();      /* effect-boundary error */
x = produce()[0];    /* effect-boundary error */
x = produce() + 0;   /* effect-boundary error */
```

An indirect call (through a function pointer) is conservatively effectful:
it must form a complete transition or a result binding, exactly like a direct
effectful call.
