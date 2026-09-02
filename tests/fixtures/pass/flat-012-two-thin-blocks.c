/* green: lint=pass matrix=pass format=pass */

void step(int n);

int run(int n);

int run(int n)
{
    if (n > 1)
    {
        step(1);
    }
    if (n > 2)
    {
        step(2);
    }
    return 0;
}
