/* green: lint=pass */

struct pt
{
    int x;
    int y;
};

int read_pt(struct pt p)
{
    int r;

    r = p.x + p.y;
    return r;
}
