/* green: lint=pass */

int stage(int s)
{
    int r;

    r = 0;
    switch (s)
    {
    case 1:
        r = 1;
        /* fall through */
    case 2:
        r = r + 1;
        break;
    default:
        r = 0;
        break;
    }
    return r;
}
