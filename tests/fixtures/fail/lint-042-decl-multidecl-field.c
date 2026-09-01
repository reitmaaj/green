/* green: lint=green-declaration */

struct pair
{
    int a, b;
};

int f(struct pair p)
{
    int r;

    r = p.a + p.b;
    return r;
}
