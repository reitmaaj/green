/* green: lint=pass matrix=pass format=pass */

#define ZEROED {0, 0}

int f(void);

int f(void)
{
    int a[2] = ZEROED;
    int r;

    r = a[0] + a[1];
    return r;
}
