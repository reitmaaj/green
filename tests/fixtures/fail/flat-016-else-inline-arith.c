/* green: lint=green-flat */

int run(int n)
{
    int s;

    s = 0;
    if (n >= 0)
    {
        s = 1;
    }
    else
    {
        s = s + n;
    }
    return s;
}
