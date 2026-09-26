/* green: lint=pass matrix=pass format=pass */

#define GREEN_PURE

GREEN_PURE
int add_one(int x);

GREEN_PURE
int add_two(int x);

int use(int a);

GREEN_PURE
int add_one(int x)
{
    return x + 1;
}

GREEN_PURE
int add_two(int x)
{
    return x + 2;
}

int use(int a)
{
    int r;

    r = add_one(a) + add_two(a);
    return r;
}
