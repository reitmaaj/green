/* green: lint=pass matrix=pass format=pass */

int cap(int a);

int cap(int a)
{
    int b;

    b = a + 1;
    if (b > 10)
    {
        return b;
    }
    return 0;
}
