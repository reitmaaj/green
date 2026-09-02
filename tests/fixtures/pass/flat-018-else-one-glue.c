/* green: lint=pass matrix=pass format=pass */

int pull(void);

int run(int n);

int run(int n)
{
    int y;

    if (n >= 0)
    {
        y = 0;
    }
    else
    {
        y = pull();
    }
    return y;
}
