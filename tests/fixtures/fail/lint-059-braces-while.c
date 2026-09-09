/* green: lint=green-braces msg="wrapped in braces" */

int f(int n)
{
    int r;

    r = 0;
    while (n > 0)
        r = r + 1;
    return r;
}
