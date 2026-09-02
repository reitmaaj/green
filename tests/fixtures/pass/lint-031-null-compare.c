/* green: lint=green-outline */

#include <stddef.h>

struct node
{
    int v;
};

int check(struct node *p)
{
    int r;

    r = 0;
    if (p == NULL)
    {
        r = 1;
    }
    return r;
}
