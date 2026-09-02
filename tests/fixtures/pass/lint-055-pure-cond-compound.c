/* green: lint=green-outline */

int clamp(int x, int lo, int hi)
{
    int r;

    if (x < lo)
    {
        r = lo;
    }
    else if (x > hi)
    {
        r = hi;
    }
    else
    {
        r = x;
    }
    return r;
}
