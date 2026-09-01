/* green: lint=pass */

#define GREEN_PURE

GREEN_PURE
int identity(int x)
{
    return x;
}

int call(int a)
{
    int r;

    r = identity(a);
    return r;
}
