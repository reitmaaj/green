/* green: lint=pass */

int sign_change(int v)
{
    unsigned int u;
    int r;

    u = (unsigned int)v;
    r = (int)u;
    return r;
}
