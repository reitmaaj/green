/* green: lint=green-pure-contract msg="false PURE annotation: body contains an effectful call" */

#define GREEN_PURE

int read_record(int fd);

GREEN_PURE
int bad(int fd)
{
    return read_record(fd);
}
