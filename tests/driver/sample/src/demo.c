#include <stddef.h>

#define GREEN_PURE

struct node
{
    int value;
};

int smaller(int a, int b);
int magnitude(int x);
int node_nonnull(struct node *p);

int smaller(int a, int b)
{
    if (a < b)
    {
        return a;
    }
    return b;
}

GREEN_PURE
int magnitude(int x)
{
    if (x < 0)
    {
        return -x;
    }
    return x;
}

int node_nonnull(struct node *p)
{
    if (p != NULL)
    {
        return 1;
    }
    return 0;
}
