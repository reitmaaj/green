/* green: lint=green-declaration */

int g_a, g_b;

int f(void)
{
    int r;

    r = g_a + g_b;
    return r;
}
