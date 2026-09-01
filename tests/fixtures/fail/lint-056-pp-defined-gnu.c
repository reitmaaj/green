/* green: lint=green-toolchain-branching */

int f(int x)
{
    int r;

    r = x;
#if defined(__GNUC__)
    r = r + 1;
#endif
    return r;
}
