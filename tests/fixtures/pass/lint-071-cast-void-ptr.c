/* green: lint=pass matrix=pass format=pass */

struct node
{
    int v;
};

int f(void *raw);

int f(void *raw)
{
    struct node *n;
    int r;

    n = (struct node *)raw;
    r = n->v;
    return r;
}
