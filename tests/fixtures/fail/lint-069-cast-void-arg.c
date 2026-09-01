/* green: lint=green-cast-boundary */

void take(void *p);

struct node
{
    int v;
};

void f(struct node *n)
{
    take((void *)n);
}
