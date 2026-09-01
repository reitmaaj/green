#define GREEN_PURE
int g_counter;

GREEN_PURE
int bump(void)
{
    g_counter = g_counter + 1;
    return g_counter;
}
