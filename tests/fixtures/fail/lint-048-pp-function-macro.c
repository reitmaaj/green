/* green: lint=green-preprocessor msg="function-like macros are forbidden" */

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int f(int a, int b)
{
    int r;

    r = MAX(a, b);
    return r;
}
