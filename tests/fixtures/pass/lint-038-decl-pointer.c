/* green: lint=pass */

#include <stddef.h>

struct node
{
    struct node *next;
};

int f(struct node *p)
{
    int r;

    r = 0;
    if (p != NULL)
    {
        r = 1;
    }
    return r;
}
