/* green: lint=green-pure-contract */

#define GREEN_PURE

int read_record(int fd);

GREEN_PURE
int bad(int fd)
{
    return read_record(fd);
}
