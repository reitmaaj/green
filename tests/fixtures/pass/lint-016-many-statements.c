/* green: lint=pass */

int many(int a, int b)
{
    int x;
    int y;
    int z;

    x = a;
    y = b;
    z = x + y;
    x = z;
    return x;
}
