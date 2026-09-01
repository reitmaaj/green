/* green: lint=green-effect-boundary */

int next(void);

struct box
{
    int value;
};

int f(void)
{
    struct box b = {next()};
    return b.value;
}
