/* green: lint=pass */

int f(void);

int g(void)
{
    (f());
    return 0;
}
