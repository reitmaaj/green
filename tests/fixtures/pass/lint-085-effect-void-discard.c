/* green: lint=pass */

int f(void);

int g(void)
{
    (void)f();
    return 0;
}
