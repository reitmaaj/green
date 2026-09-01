/* green: lint=green-cast-boundary */

int f(int *p)
{
    unsigned long addr;
    int r;

    addr = (unsigned long)p;
    r = (int)addr;
    return r;
}
