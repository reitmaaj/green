/* green: lint=green-cast-boundary */

struct node
{
    int v;
};

int f(void *raw)
{
    struct node *n;
    int r;

    n = (struct node *)raw;
    r = n->v;
    return r;
}
