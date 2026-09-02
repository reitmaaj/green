/* green: lint=green-flat */

int run(int n)
{
    int s;

    s = 0;
    {
        int x = n * 2;

        s = x;
    }
    return s;
}
