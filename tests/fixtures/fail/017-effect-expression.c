int read_record(int);

int f(int fd)
{
    int n;
    n = 0;
    n = read_record(fd) + 1;
    return n;
}
