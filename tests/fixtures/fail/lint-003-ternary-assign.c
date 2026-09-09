/* green: lint=green-hidden-control msg="if/else that assigns" */

int f(int c, int a, int b)
{
    int r;

    r = c ? a : b;
    return r;
}
