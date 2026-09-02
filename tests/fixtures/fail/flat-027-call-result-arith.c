/* green: lint=green-flat */

int pick(void);

int run(int n);

int run(int n)
{
    int x;

    x = 0;
    if (n > 0)
    {
        x = pick() + 1;
    }
    return x;
}
