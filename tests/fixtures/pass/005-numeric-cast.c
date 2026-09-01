int narrow(unsigned long size)
{
    unsigned int n;
    unsigned char byte;

    n = (unsigned int)size;
    byte = (unsigned char)n;
    return (int)byte;
}
