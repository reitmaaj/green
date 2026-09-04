/* green: lint=green-effect-boundary */

int produce(void);

int compute(int n)
{
    int x;

    x = (produce()) + 1;
    return x;
}
