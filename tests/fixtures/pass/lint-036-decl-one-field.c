/* green: lint=pass */

struct pair
{
    int a;
    int b;
};

int f(struct pair p)
{
    int r;

    r = p.a + p.b;
    return r;
}
