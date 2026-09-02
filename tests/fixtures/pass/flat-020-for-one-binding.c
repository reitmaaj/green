/* green: lint=pass matrix=pass format=pass */

int pull(void);

int run(int n);

int run(int n)
{
    int i;
    int x;

    x = 0;
    for (i = 0; i < n; ++i)
    {
        x = pull();
    }
    return x;
}
