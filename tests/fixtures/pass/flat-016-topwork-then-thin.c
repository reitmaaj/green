/* green: lint=pass matrix=pass format=pass */

int run(int n);

int run(int n)
{
    int x;
    int y;

    x = n * n + n / 2;
    y = x * 3;
    if (y > 100)
    {
        x = 0;
    }
    return x;
}
