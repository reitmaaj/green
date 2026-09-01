/* green: lint=pass */

int neg_abs(int x)
{
    int r;

    r = 0;
    if (x < 0)
    {
        r = -x;
    }
    else
    {
        r = +x;
    }
    return r;
}
