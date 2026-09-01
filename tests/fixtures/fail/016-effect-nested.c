int read_record(int);
int consume(int);

int f(int fd)
{
    int r;
    r = 0;
    r = consume(read_record(fd));
    return r;
}
