/* green: lint=pass */

int f(int a, int b)
{
    int r;

    r = 0;
    if (a)
    {
        if (b)
        {
            r = 1;
        }
    }
    return r;
}
