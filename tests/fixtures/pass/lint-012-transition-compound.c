/* green: lint=pass */

int compound(int x, int n)
{
    int r;

    r = x;
    r += n;
    r -= 1;
    return r;
}
