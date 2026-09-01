/* green: matrix=pass */

int down(int n);

int down(int n)
{
    int s;

    s = 0;
    while (n > 0)
    {
        s = s + n;
        --n;
    }
    return s;
}
