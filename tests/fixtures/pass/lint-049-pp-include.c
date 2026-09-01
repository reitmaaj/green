/* green: lint=pass */

#include <stddef.h>

int f(void)
{
    int r;

    r = (int)sizeof(size_t);
    return r;
}
