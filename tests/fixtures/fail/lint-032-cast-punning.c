/* green: lint=green-cast-boundary */

struct a
{
    int x;
};
struct b
{
    int y;
};

int f(struct a *pa)
{
    struct b *pb;
    int r;

    pb = (struct b *)pa;
    r = pb->y;
    return r;
}
