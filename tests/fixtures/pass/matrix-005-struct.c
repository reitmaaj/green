/* green: matrix=pass */

struct pt
{
    int x;
    int y;
};

int norm(struct pt p);

int norm(struct pt p)
{
    return p.x + p.y;
}
