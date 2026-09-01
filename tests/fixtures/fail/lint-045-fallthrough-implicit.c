/* green: lint=green-fallthrough */

int f(int s)
{
    int r;

    r = 0;
    switch (s)
    {
    case 1:
        r = 1;
    case 2:
        r = 2;
        break;
    default:
        r = 3;
        break;
    }
    return r;
}
