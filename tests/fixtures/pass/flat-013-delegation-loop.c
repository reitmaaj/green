/* green: lint=pass matrix=pass format=pass */

int add(int a, int b);

int run(int n);

int add(int a, int b)
{
    return a + b;
}

int run(int n)
{
    int i;
    int s;

    s = 0;
    for (i = 0; i < n; ++i)
    {
        s = add(s, i);
    }
    return s;
}
