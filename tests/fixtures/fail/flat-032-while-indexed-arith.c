/* green: lint=green-flat */

int run(int *v, int n);

int run(int *v, int n)
{
    int i;
    int s;

    s = 0;
    i = 0;
    while (i < n)
    {
        s = s + v[i];
        ++i;
    }
    return s;
}
