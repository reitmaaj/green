/* green: lint=pass */

struct foo
{
    int value;
};

void *allocate(void);

struct foo *make(void)
{
    struct foo *p;

    p = (struct foo *)allocate();
    return p;
}
