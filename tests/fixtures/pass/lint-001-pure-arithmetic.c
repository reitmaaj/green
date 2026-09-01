/* green: lint=pass */

int pure_terms(int a, int b, int c)
{
    int x;
    int y;

    x = (a + b) * (c - a);
    y = ((a << 2) & b) != 0;
    return x + y;
}
