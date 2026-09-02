/* green: lint=pass matrix=pass format=pass */

#define GREEN_PURE

GREEN_PURE
int pick(int a);

int run(int n);

GREEN_PURE
int pick(int a)
{
    return a * 2;
}

int run(int n)
{
    if (n > 0)
    {
        int z = pick(n);

        return z;
    }
    return 0;
}
