/* green: lint=green-preprocessor */

#include "../external/ext.h"

#define OUTER EXT_AND

int f(int a, int b)
{
    int r;

    r = OUTER(a, b);
    return r;
}
