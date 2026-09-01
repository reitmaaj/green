/* green: lint=green-transition-boundary */

int f(int y, int z)
{
    int x;

    x = (y = z);
    return x;
}
