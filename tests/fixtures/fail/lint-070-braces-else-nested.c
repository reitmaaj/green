/* green: lint=green-braces msg="wrapped in braces" */

int f(int a, int b)
{
    int r;

    r = 0;
    if (a)
    {
        if (b)
        {
            r = 1;
        }
        else
            r = 2;
    }
    return r;
}
