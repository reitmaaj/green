/* green: lint=green-fallthrough */

int f(int s, int c)
{
    int r;

    r = 0;
    switch (s)
    {
    case 1:
        if (c)
        {
            r = 1;
        }
        else
        {
            r = 2;
        }
    case 2:
        r = r + 1;
        break;
    default:
        r = 0;
        break;
    }
    return r;
}
