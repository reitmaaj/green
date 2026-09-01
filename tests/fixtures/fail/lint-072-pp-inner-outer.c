/* green: lint=green-preprocessor */

#define INNER (a && b)
#define OUTER INNER

int f(int a, int b)
{
    int r;

    r = 0;
    if (OUTER)
    {
        r = 1;
    }
    return r;
}
