/* green: lint=readability-braces-around-statements */

int f(int c)
{
    int r;

    r = 0;
    if (c)
    {
        r = 1;
    }
    else
        r = 2;
    return r;
}
