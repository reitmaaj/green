/* green: matrix=pass */

int mask(unsigned int v);

int mask(unsigned int v)
{
    int r;

    r = (int)(v & 0xFFu);
    return r;
}
