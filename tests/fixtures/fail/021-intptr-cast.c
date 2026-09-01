int f(unsigned long addr)
{
    int *p;
    p = (int *)addr;
    return *p;
}
