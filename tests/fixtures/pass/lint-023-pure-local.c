/* green: lint=pass */

#define GREEN_PURE

GREEN_PURE
int square(int x)
{
    int r;

    r = x * x;
    return r;
}
