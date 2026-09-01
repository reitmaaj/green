int step(int *p)
{
    int i;
    int n;

    i = 0;
    n = 4;
    p[0] = 0;
    ++i;
    --n;
    return i + n + p[0];
}
