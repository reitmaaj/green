/* green: lint=green-cast-boundary */

int f(int *ip)
{
    void (*fn)(void);
    int r;

    r = 0;
    fn = (void (*)(void))ip;
    return r;
}
