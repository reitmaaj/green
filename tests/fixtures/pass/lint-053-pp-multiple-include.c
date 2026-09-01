/* green: lint=pass */

#include <stddef.h>
#include <limits.h>

int f(void)
{
    int r;

    r = (int)sizeof(size_t) + INT_MAX;
    return r;
}
