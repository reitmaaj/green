/* green: lint=green-effect-boundary */

int read_record(int fd);

int f(int fd)
{
    int n;

    n = 0;
    n = read_record(fd) + 1;
    return n;
}
