/* green: lint=green-effect-boundary */

int produce(void);
int *produce_ptr(void);
int *items(void);

int neg(void)
{
    int x;

    x = -produce();
    return x;
}

int deref(void)
{
    int x;

    x = *produce_ptr();
    return x;
}

int index_at(int i)
{
    int x;

    x = items()[i];
    return x;
}
