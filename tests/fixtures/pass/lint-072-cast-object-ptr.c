/* green: lint=pass matrix=pass format=pass */

struct a
{
    int x;
};
struct b
{
    int y;
};

int f(struct a *pa);

int f(struct a *pa)
{
    struct b *pb;
    int r;

    pb = (struct b *)pa;
    r = pb->y;
    return r;
}
