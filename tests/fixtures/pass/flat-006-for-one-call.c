/* green: lint=pass matrix=pass format=pass */

void feed(int n);

int run(int n);

int run(int n)
{
    int i;

    for (i = 0; i < n; ++i)
    {
        feed(i);
    }
    return 0;
}
