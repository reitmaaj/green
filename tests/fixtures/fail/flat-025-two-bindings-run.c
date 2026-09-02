/* green: lint=green-flat */

int pull(void);

int run(int n);

int run(int n)
{
    int x;
    int y;

    if (n > 0)
    {
        x = pull();
        y = pull();
    }
    return x + y;
}
