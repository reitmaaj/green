/* green: lint=green-flat */

void step(void);

int run(int n);

int run(int n)
{
    int s;

    s = 0;
    if (n > 0)
    {
        step();
        s = s + n;
    }
    return s;
}
