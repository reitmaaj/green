/* green: lint=green-braces msg="wrapped in braces" */

int f(int c)
{
    int r;

    r = 0;
    if (c)
        r = 1;
    return r;
}
