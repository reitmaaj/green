/* green: lint=pass matrix=pass format=pass */

enum f_pc
{
    F_B0,
    F_DONE
};

struct f_frame
{
    int x;
    int result;
};

int f(int x);

static enum f_pc f_b0(struct f_frame *s)
{
    s->x = s->x + 1;
    if (s->x > 10)
    {
        return F_DONE;
    }
    return F_B0;
}

int f(int x)
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
        case F_DONE:
            return s.result;
        }
    }
}
