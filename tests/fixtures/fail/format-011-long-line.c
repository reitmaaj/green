/* green: format=fail */

int f(int a, int b, int c, int d, int e)
{
    int r;

    r = a + b + c + d + e + a + b + c + d + e + a + b + c + d + e + a + b + c + d + e;
    return r;
}
