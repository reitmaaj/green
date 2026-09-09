/* green: lint=green-transition-boundary msg="standalone statement" */

int f(int y, int z)
{
    int x;

    x = (y = z);
    return x;
}
