#include <stddef.h>

#define GREEN_PURE

struct node
{
    int value;
};

int pure_abs(int x);
int process(int n, int *out);

GREEN_PURE
int pure_abs(int x)
{
    int y;
    if (x < 0)
    {
        y = -x;
    }
    else
    {
        y = x;
    }
    return y;
}

int process(int n, int *out)
{
    int i;
    int sum;
    struct node *nptr;

    sum = 0;
    nptr = NULL;
    for (i = 0; i < n; ++i)
    {
        sum = sum + out[i];
    }
    if (nptr != NULL)
    {
        sum = sum + nptr->value;
    }
    if (sum < 0)
    {
        sum = pure_abs(sum);
    }
    return sum;
}
