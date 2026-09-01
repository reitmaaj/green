/* green: lint=pass */

int read_byte(int fd);

int next(int fd)
{
    int b;

    b = read_byte(fd);
    return b;
}
