/* green: lint=green-effect-boundary */

int ext(int);

int (*op)(int) = ext;

int decide(int a)
{
    if (op(a))
    {
        return 1;
    }
    return 0;
}
