/* green: lint=green-cast-boundary msg="integer/pointer" */

int f(unsigned long addr)
{
    int *p;
    int r;

    p = (int *)addr;
    r = *p;
    return r;
}
