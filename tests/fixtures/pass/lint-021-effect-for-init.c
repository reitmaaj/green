/* green: lint=green-outline */

int next(void);

int take_first(void)
{
    int v;
    int i;

    v = 0;
    for (i = next(); i < 10; ++i)
    {
        v = v + i;
    }
    return v;
}
