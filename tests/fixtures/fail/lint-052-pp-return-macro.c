/* green: lint=green-preprocessor */

#define EARLY return 1

int f(int x)
{
    int r;

    r = 0;
    EARLY;
    return r;
}
