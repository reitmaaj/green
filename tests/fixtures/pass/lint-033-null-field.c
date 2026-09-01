/* green: lint=pass */

#include <stddef.h>

struct list
{
    struct list *next;
};

struct list *head(void)
{
    struct list n;
    int r;

    n.next = NULL;
    r = 0;
    return n.next;
}
