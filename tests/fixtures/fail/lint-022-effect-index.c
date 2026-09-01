/* green: lint=green-effect-boundary */

int read_record(int fd);

int f(int fd, int *v)
{
    int r;

    r = 0;
    r = v[read_record(fd)];
    return r;
}
