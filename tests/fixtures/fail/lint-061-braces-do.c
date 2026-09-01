/* green: lint=readability-braces-around-statements */

int f(int n)
{
    int r;

    r = 0;
    do
        r = r + 1;
    while (n > 0);
    return r;
}
