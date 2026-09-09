/* green: lint=green-hidden-control msg="if (a) { if (b)" msg="short-circuit" */

int f(int a, int b)
{
    int r;

    r = 0;
    if (a && b)
    {
        r = 1;
    }
    return r;
}
