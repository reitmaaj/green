/* green: lint=green-flat */

int run(int n)
{
    int i;
    int s;

    s = 0;
    i = 0;
    while (i < n)
    {
        ++i;
        if (i > 2)
        {
            s = s + i;
        }
    }
    return s;
}
