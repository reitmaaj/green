/* green: lint=pass matrix=pass format=pass */

#define BASE (7 + 8)

int f(int n);

int f(int n)
{
    int r;

    r = n * (n - BASE);
    return r;
}
