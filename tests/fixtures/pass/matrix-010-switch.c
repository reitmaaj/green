/* green: matrix=pass */

int classify(int s);

int classify(int s)
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
