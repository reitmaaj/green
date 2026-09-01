/* green: lint=pass matrix=pass format=pass */

void take(void *p);

struct node
{
    int v;
};

void f(struct node *n);

void f(struct node *n)
{
    take((void *)n);
}
