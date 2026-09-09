/* green: lint=green-null msg="use NULL for a null pointer constant" msg="stddef.h" */

struct node
{
    int v;
};

int f(struct node *p)
{
    int r;

    r = 0;
    p = 0;
    return r;
}
