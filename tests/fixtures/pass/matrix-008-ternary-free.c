/* green: matrix=pass */

int clamp(int x);

int clamp(int x)
{
    if (x < 0)
    {
        return 0;
    }
    return x;
}
