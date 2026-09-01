/* green: lint=green-hidden-control */

int f(int n, int go)
{
    int i;
    int r;

    r = 0;
    for (i = 0; i < n || go; ++i)
    {
        r = r + i;
    }
    return r;
}
