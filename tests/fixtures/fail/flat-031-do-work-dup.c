/* green: lint=green-flat */

int run(int n);

int run(int n)
{
    int i;
    int s;

    s = 0;
    i = 0;
    do
    {
        s = s - i;
        ++i;
    } while (i < n);
    return s;
}
