/* green: lint=green-toolchain-branching msg="compatibility_paths" */

int f(int x)
{
    int r;

    r = x;
#ifdef __GNUC__
    r = r + 1;
#endif
    return r;
}
