/* green: lint=green-transition-boundary */

int f(int x, int y, int z)
{
    int r;

    r = x;
    r += (y += z);
    return r;
}
