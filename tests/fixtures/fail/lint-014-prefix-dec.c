/* green: lint=green-flat */

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
