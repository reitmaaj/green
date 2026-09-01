/* green: lint=pass */

int count_up(int n)
{
    int i;
    int s;

    s = 0;
    i = 0;
    while (i < n)
    {
        s = s + i;
        ++i;
    }
    return s;
}
