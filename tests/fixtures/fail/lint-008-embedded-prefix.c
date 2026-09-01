/* green: lint=green-transition-boundary */

int f(int n)
{
    int i;
    int x;

    i = 0;
    x = ++i;
    return x + n;
}
