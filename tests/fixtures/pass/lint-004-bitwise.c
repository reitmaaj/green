/* green: lint=pass */

int bits(unsigned int flags)
{
    unsigned int mask;
    int r;

    mask = ((flags << 3) & 0xFFu) ^ 0x1u;
    r = (int)mask;
    return r;
}
