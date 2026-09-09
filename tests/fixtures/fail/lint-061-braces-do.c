/* green: lint=green-braces msg="wrapped in braces" */

int f(int n)
{
    int r;

    r = 0;
    do
        r = r + 1;
    while (n > 0);
    return r;
}
