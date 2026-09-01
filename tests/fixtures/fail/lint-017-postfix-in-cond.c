/* green: lint=green-transition-boundary */

int f(int n)
{
    int i;
    int r;

    r = 0;
    i = 0;
    while (i++ < n)
    {
        r = r + 1;
    }
    return r;
}
