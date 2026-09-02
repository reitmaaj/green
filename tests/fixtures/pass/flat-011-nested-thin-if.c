/* green: lint=pass matrix=pass format=pass */

void step(int n);

int run(int n);

int run(int n)
{
    if (n > 0)
    {
        if (n > 1)
        {
            step(n);
        }
    }
    return 0;
}
