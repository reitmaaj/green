/* green: lint=green-effect-boundary */

int read_record(int fd);

int f(int fd)
{
    int r;

    r = 0;
    if (read_record(fd))
    {
        r = 1;
    }
    return r;
}
