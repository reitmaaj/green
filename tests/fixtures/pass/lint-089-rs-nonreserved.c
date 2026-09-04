/* green: lint=pass */

struct point
{
    int x;
    int y;
};

typedef struct point Point;

enum color
{
    COLOR_RED,
    COLOR_GREEN
};

int abscissa(Point p)
{
    return p.x;
}
