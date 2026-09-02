/* green: lint=green-flat */

int run(int n);

int run(int n)
{
    int i;
    int s;

    s = 0;
    for (i = 0; i < n; ++i)
    {
        int y = n * i;

        s = y;
    }
    return s;
}
