/* green: lint=green-effect-boundary */

int produce(void);

void scan(int n)
{
    int i;

    for (i = 0; produce(); ++i)
    {
    }
}
