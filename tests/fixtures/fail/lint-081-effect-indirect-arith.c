/* green: lint=green-effect-boundary */

int ext(int);

int (*op)(int) = ext;

int compute(int a)
{
    int r;

    r = op(a) + 1;
    return r;
}
