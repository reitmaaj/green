#include <stddef.h>

struct node
{
    int v;
};

int use(struct node *p)
{
    int r;
    if (p == NULL)
    {
        r = 0;
    }
    else
    {
        r = p->v;
    }
    return r;
}
