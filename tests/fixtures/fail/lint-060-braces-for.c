/* green: lint=readability-braces-around-statements */

int f(int n)
{
    int i;
    int r;

    r = 0;
    for (i = 0; i < n; ++i)
        r = r + i;
    return r;
}
