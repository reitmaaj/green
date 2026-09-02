/* green: lint=green-outline */

int cmp(int a, int b)
{
    int r;

    r = 0;
    if (a < b)
    {
        r = 1;
    }
    if (b > 0)
    {
        r = 2;
    }
    return r;
}
