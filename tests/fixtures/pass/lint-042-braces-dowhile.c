/* green: lint=pass */

int once(int n)
{
    int s;

    s = 0;
    do
    {
        s = s + 1;
        --n;
    } while (n > 0);
    return s;
}
