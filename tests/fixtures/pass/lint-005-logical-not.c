/* green: lint=green-outline */

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
