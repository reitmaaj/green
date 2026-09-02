/* green: lint=green-flat */

int grid(int n, int m)
{
    int i;
    int j;
    int s;

    s = 0;
    for (i = 0; i < n; ++i)
    {
        j = 0;
        while (j < m)
        {
            s = s + i * j;
            ++j;
        }
    }
    return s;
}
