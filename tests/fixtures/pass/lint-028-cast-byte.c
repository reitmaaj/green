/* green: lint=pass */

int byte_of(int value)
{
    unsigned char b;
    int r;

    b = (unsigned char)value;
    r = (int)b;
    return r;
}
