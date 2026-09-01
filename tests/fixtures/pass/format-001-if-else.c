/* green: format=pass */

int classify(int x)
{
    int r;

    r = 0;
    if (x < 0)
    {
        r = -1;
    }
    else
    {
        r = 1;
    }
    return r;
}
