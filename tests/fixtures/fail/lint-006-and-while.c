/* green: lint=green-hidden-control */

int f(int a, int b)
{
    int r;

    r = 0;
    while (a && b)
    {
        r = r + 1;
        --a;
    }
    return r;
}
