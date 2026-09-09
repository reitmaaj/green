/* green: lint=green-cast-boundary msg="remove the cast" */

int f(int v)
{
    int x;

    x = (int)v;
    return x;
}
