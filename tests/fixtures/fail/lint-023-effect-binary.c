/* green: lint=green-effect-boundary */

int read_record(int fd);

int f(int fd, int x)
{
    int r;

    r = 0;
    r = x * read_record(fd);
    return r;
}
