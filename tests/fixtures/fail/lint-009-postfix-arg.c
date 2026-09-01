/* green: lint=green-transition-boundary */

void consume(int i);

int f(int n)
{
    int i;

    i = 0;
    consume(i++);
    return n;
}
