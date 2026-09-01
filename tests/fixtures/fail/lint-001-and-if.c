/* green: lint=green-hidden-control */

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
