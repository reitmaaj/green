/* green: lint=green-flat */

int f(int x)
{
    int y;

    y = 0;
    {
        y = x * 2;
    }
    return y;
}
