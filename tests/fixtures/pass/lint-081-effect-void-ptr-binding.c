/* green: lint=pass */

struct node
{
    int value;
};

void *allocate(void);

int load(void)
{
    struct node *p;
    int v;

    p = allocate();
    v = p->value;
    return v;
}
