/* green: lint=green-cast-boundary */

int f(unsigned long addr)
{
    int *p;
    int r;

    p = (int *)addr;
    r = *p;
    return r;
}
