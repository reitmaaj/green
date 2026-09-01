/* green: lint=green-hidden-control */

int f(int c, int a, int b)
{
    int r;

    r = c ? a : b;
    return r;
}
