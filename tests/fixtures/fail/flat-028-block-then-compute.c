/* green: lint=green-flat */

int run(int a);

int run(int a)
{
    int x;

    {
        x = a;
        x = x * 2;
    }
    return x;
}
