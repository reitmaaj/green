/* green: lint=green-toolchain-branching */

int f(int x)
{
    int r;

    r = x;
#ifdef __clang__
    r = r + 1;
#endif
    return r;
}
