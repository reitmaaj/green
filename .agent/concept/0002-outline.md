# green outline — basic-block extraction as a first-class capability

`green` enforces a strict C89 ∩ C23 source profile. This concept adds a
single idea: **any owned function whose body contains more than one basic
block must be outlined** — each source-level basic block becomes a
file-local helper function and all inter-block transfer happens through an
explicit program counter. Outlining is performed by `green` itself; the
tool is both the transformer and the gate. It is not a separate downstream
tool.

Terminology: use **outline** / **extract**; do not use "lift".

## Central thesis

- Control flow authority is `clang::CFG`; block identity is a
  `clang::CFGBlock` (possibly coalesced to maximal single-entry
  source-emittable regions that never split an indivisible C full
  expression).
- `green` already forces statement-level explicit transitions and forbids
  hidden control (`?:`, `&&`, `||`, comma) in expression position.
  Outlined output MUST therefore conform to green rather than green
  conforming to the outliner.
- A function is outlined when it is a dispatcher that only sequences
  single-block helpers through an explicit program counter. Such output is
  idempotent: re-outlining it is a no-op.

## Representative shape

Straight-line single-block functions are accepted unchanged. A multi-block
function is outlined into:

```c
enum f_pc
{
    F_B0,
    F_B1,
    F_DONE
};

struct f_frame
{
    int x;
    int y;
    int result;
};

static enum f_pc
f_b0(struct f_frame *s)
{
    s->y = s->x + 1;

    if (s->y > 10)
    {
        return F_B1;
    }
    return F_B0;
}

static enum f_pc
f_b2(struct f_frame *s)
{
    s->result = s->y;
    return F_DONE;
}

int
f(int x)
{
    struct f_frame s;
    enum f_pc pc;

    s.x = x;
    pc = F_B0;

    for (;;)
    {
        switch (pc)
        {
        case F_B0:
            pc = f_b0(&s);
            break;
        case F_B2:
            pc = f_b2(&s);
            break;
        case F_DONE:
            return s.result;
        }
    }
}
```

Exact emitted spelling may differ; the semantic model does not. Output MUST
be green-clean: no `?:`, no hidden control, Allman braces with mandatory
braces on every controlled body, and canonical under the green format
profile.
