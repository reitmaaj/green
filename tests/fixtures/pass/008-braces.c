int classify(int x)
{
    int r;
    if (x < 0)
    {
        r = -1;
    }
    else if (x == 0)
    {
        r = 0;
    }
    else
    {
        r = 1;
    }
    return r;
}
