/* green: lint=green-outline */

#include <stddef.h>

struct node
{
    struct node *next;
};

int valid(struct node *n)
{
    int r;

    r = 0;
    if (n->next != NULL)
    {
        r = 1;
    }
    return r;
}
