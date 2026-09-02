/* green: lint=green-flat */

int run(int n)
{
    int s;

    s = 0;
    switch (n)
    {
    case 1:
    {
        s = s + 1;
        break;
    }
    default:
        s = 0;
        break;
    }
    return s;
}
