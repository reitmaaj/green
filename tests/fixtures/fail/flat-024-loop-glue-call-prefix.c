/* green: lint=green-flat */

void step(void);

int run(int n);

int run(int n)
{
    while (n > 0)
    {
        step();
        --n;
    }
    return 0;
}
