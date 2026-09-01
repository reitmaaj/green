/* green: lint=green-pure-contract */

#define GREEN_PURE

int external_helper(int x);

GREEN_PURE
int bad(int x)
{
    return external_helper(x);
}
