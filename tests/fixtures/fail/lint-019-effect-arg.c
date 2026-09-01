/* green: lint=green-effect-boundary */

int read_record(int fd);
void consume(int n);

int f(int fd)
{
    int r;

    r = 0;
    consume(read_record(fd));
    return r;
}
