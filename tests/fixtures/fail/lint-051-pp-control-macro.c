/* green: lint=green-preprocessor */

#define CHECK(x) if (x) { return 1; }

int f(int x)
{
    int r;

    r = 0;
    CHECK(x)
    return r;
}
