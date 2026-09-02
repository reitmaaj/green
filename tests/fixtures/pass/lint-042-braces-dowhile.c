/* green: lint=green-outline */

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
