/* green: lint=pass */

int extract(unsigned int word)
{
    unsigned int part;
    int r;

    part = (word >> 4) & 0xFFu;
    r = (int)part;
    return r;
}
