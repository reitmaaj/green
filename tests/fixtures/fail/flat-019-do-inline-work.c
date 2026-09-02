/* green: lint=green-flat */

int run(int n)
{
    int s;

    s = 0;
    do
    {
        s = s + n;
        --n;
    } while (n > 0);
    return s;
}
