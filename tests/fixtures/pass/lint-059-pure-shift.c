/* green: lint=pass */

unsigned int mask(unsigned int flags)
{
    unsigned int r;

    r = flags << 8;
    return r;
}
