/* green: lint=green-null */

struct node
{
    int v;
};

int f(struct node **slot)
{
    int r;

    r = 0;
    *slot = (void *)0;
    return r;
}
