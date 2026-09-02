/* green: lint=pass matrix=pass format=pass */

int pull(void);

int run(int n);

int run(int n)
{
    int x;

    x = n;
    while (x > 0)
    {
        x = pull();
    }
    return x;
}
