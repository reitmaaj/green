/* green: lint=green-cast-boundary */

struct node
{
    int v;
};

struct node *make(void *raw)
{
    return (struct node *)raw;
}
