/* green: lint=pass */

int spin(int *ready)
{
    int r;

    r = 0;
    while (*ready == 0)
    {
    }
    return r;
}
