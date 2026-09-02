/* green: lint=pass matrix=pass format=pass */

int pull(void);

int run(int n);

int run(int n)
{
    int y;

    y = 0;
    do
    {
        y = pull();
    } while (y < n);
    return y;
}
