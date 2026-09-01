/* green: lint=green-hidden-control */

int f(int a, int b)
{
    int r;

    r = (a, b);
    return r;
}
