/* green: lint=pass */

int not_zero(int a)
{
    int r;

    r = 0;
    if (!a)
    {
        r = 1;
    }
    return r;
}
