/* green: lint=green-hidden-control */

int f(int c, int n)
{
    int r;

    r = 0;
    while (n ? c : 0)
    {
        r = r + 1;
        --n;
    }
    return r;
}
