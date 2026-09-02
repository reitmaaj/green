/* green: lint=pass matrix=pass format=pass */

void left(int n);

void right(int n);

int run(int n);

int run(int n)
{
    if (n > 0)
    {
        left(n);
    }
    else
    {
        right(n);
    }
    return 0;
}
