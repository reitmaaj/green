/* green: lint=green-braces msg="must be '{}', not ';'" */

int f(int *ready)
{
    int r;

    r = 0;
    while (*ready == 0)
        ;
    return r;
}
