/* green: lint=green-pure-contract msg="a volatile access" */

#define GREEN_PURE

volatile int g_flag;

GREEN_PURE
int bad(void)
{
    return g_flag;
}
