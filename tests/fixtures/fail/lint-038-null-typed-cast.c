/* green: lint=green-null */

struct node
{
    int v;
};

int f(struct node *p)
{
    int r;

    r = 0;
    p = (struct node *)0;
    return r;
}
