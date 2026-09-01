/* green: lint=green-hidden-control */

int f(int n, int ready)
{
    int i;
    int r;

    r = 0;
    for (i = 0; i < n && ready; ++i)
    {
        r = r + i;
    }
    return r;
}
