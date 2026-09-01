/* green: lint=pass */

int deep_pure(int a, int b, int c, int d)
{
    int x;

    x = ((a + b) * (c - d)) + (a * d);
    return x;
}
