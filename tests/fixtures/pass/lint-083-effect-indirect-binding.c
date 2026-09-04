/* green: lint=pass */

int apply(int (*op)(int, int), int a, int b)
{
    int r;

    r = op(a, b);
    return r;
}

void dispatch(int (*fn)(int))
{
    fn(7);
}
