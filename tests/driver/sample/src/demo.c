#include <stddef.h>
#include <string.h>

#define GREEN_PURE

struct node
{
    int value;
};

int smaller(int a, int b);
int magnitude(int x);
int node_nonnull(struct node *p);
int total_length(const char *a, const char *b);

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

int total_length(const char *a, const char *b)
{
    size_t n;

    n = strlen(a) + strlen(b);
    if (n > 10)
    {
        return 1;
    }
    return 0;
}
