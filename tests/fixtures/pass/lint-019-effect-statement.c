/* green: lint=pass */

void write_byte(int fd, int b);

void emit(int fd, int b)
{
    write_byte(fd, b);
}
