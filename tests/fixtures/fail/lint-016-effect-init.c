/* green: lint=green-effect-boundary */

int open_file(void);

int f(void)
{
    int fd = open_file();
    return fd;
}
