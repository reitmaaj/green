/* green: lint=green-effect-boundary */

int read_record(int fd);

int f(int fd)
{
    return read_record(fd);
}
