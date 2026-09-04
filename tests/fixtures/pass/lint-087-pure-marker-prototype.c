/* green: lint=pass */

#define GREEN_PURE

GREEN_PURE
int abs_val(int x);

int total(int n)
{
    int x;

    x = abs_val(n) + abs_val(2);
    return x;
}
