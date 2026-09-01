/* green: lint=green-cast-boundary */

int f(int v)
{
    int x;

    x = (int)v;
    return x;
}
