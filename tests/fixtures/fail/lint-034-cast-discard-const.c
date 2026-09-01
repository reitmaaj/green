/* green: lint=green-cast-boundary */

int f(const char *cc)
{
    char *p;
    int r;

    r = 0;
    p = (char *)cc;
    return r;
}
