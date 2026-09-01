#define GREEN_PURE

int pure_abs(int x);

GREEN_PURE
int pure_abs(int x)
{
    int y;
    if (x < 0)
    {
        y = -x;
    }
    else
    {
        y = x;
    }
    return y;
}

int use(int a)
{
    int r;
    r = pure_abs(a) + pure_abs(a);
    return r;
}
