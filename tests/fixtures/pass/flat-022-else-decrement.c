/* green: lint=pass matrix=pass format=pass */

int run(int n);

int run(int n)
{
    if (n > 0)
    {
        n = 0;
    }
    else
    {
        --n;
    }
    return n;
}
