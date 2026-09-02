/* green: lint=green-flat */

int step(void);

int walk(int n)
{
    int i;
    int v;

    v = 0;
    i = 0;
    for (i = 0; i < n; step())
    {
        v = v + i;
        ++i;
    }
    return v;
}
