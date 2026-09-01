/* green: matrix=c89 */

int sum(int n);

int sum(int n)
{
    int s;

    s = 0;
    for (int i = 0; i < n; ++i)
    {
        s = s + i;
    }
    return s;
}
