/* green: lint=pass matrix=pass format=pass */

#include "../external/ext.h"

#define MINE (EXT_PURE + 1)

int f(void);

int f(void)
{
    int r;

    r = MINE;
    return r;
}
