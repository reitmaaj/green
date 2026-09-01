/* green: lint=pass */

int narrow(unsigned long size)
{
    unsigned int n;
    int r;

    n = (unsigned int)size;
    r = (int)n;
    return r;
}
