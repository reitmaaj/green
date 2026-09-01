/* green: lint=pass matrix=pass format=pass */

struct node
{
    int v;
};

struct node *make(void *raw);

struct node *make(void *raw)
{
    return (struct node *)raw;
}
