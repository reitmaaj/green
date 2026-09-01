/* green: lint=pass */

int hi(int v)
{
    unsigned long l;
    int r;

    l = (unsigned long)v;
    r = (int)(l >> 1);
    return r;
}
