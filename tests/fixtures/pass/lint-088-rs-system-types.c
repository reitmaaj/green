/* green: lint=pass */

#include <stdint.h>
#include <stddef.h>
#include <time.h>

size_t grow(size_t n)
{
    return n + 1;
}

int is_late(time_t now, time_t limit)
{
    return now > limit;
}
