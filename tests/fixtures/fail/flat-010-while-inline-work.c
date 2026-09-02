/* green: lint=green-flat */

int until_zero(int n)
{
    int s;

    s = 0;
    while (n > 0)
    {
        s = s + n;
        --n;
    }
    return s;
}
