/* green: lint=green-flat */

int run(int n)
{
    int s;

    s = 0;
    while (n > 0)
    {
        s += n;
        --n;
    }
    return s;
}
