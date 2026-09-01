/* green: lint=readability-braces-around-statements */

int f(int a, int b)
{
    int r;

    r = 0;
    if (a)
    {
        r = 1;
    }
    else if (b)
    {
        r = 2;
    }
    else
        r = 3;
    return r;
}
