/* green: format=pass */

int pick(int s)
{
    int r;

    r = 0;
    switch (s)
    {
    case 1:
        r = 1;
        break;
    default:
        r = 2;
        break;
    }
    return r;
}
