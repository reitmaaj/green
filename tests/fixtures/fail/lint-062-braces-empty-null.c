/* green: lint=readability-braces-around-statements */

int f(int *ready)
{
    int r;

    r = 0;
    while (*ready == 0)
        ;
    return r;
}
