/* green: lint=green-hidden-control msg="separator" msg="separate statements" */

int f(int a, int b)
{
    int r;

    r = (a, b);
    return r;
}
