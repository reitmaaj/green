/* green: lint=pass */

int g_count;
int g_limit;

int f(void)
{
    int r;

    g_count = 1;
    g_limit = 2;
    r = g_count + g_limit;
    return r;
}
