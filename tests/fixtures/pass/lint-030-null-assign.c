/* green: lint=pass */

#include <stddef.h>

struct node
{
    int v;
};

int clear(struct node **slot)
{
    int r;

    *slot = NULL;
    r = 0;
    return r;
}
