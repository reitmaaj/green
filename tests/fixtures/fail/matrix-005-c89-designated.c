/* green: matrix=c89 */

struct pt
{
    int x;
    int y;
};

int f(void);

int f(void)
{
    struct pt p = {.x = 1, .y = 2};
    return p.x + p.y;
}
