/* green: lint=pass */

int count_down(int n)
{
    int i;
    int s;

    s = 0;
    i = n;
    while (i > 0)
    {
        s = s + i;
        --i;
    }
    return s;
}
