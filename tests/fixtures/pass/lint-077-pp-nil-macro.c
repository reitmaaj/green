/* green: lint=green-outline matrix=pass format=pass */

#define NIL ((void *)0)

struct node
{
    int v;
};

int f(struct node *n);

int f(struct node *n)
{
    int r;

    r = 0;
    n = NIL;
    if (n == NIL)
    {
        r = 1;
    }
    return r;
}
