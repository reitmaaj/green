/* green: matrix=pass */

int total(int *v, int n);

int total(int *v, int n)
{
    int i;
    int s;

    s = 0;
    for (i = 0; i < n; ++i)
    {
        s = s + v[i];
    }
    return s;
}
