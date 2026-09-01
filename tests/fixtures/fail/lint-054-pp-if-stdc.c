/* green: lint=green-toolchain-branching */

int f(int x)
{
    int r;

    r = x;
#if __STDC_VERSION__ >= 201112L
    r = r + 1;
#endif
    return r;
}
