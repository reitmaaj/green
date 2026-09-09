/* green: lint=green-flat msg="worker function" */

int read_x(void);
int add(int a, int b);
int go(int n);

int go(int n)
{
    int i;

    for (i = 0; i < n; ++i)
    {
        read_x();
        add(i, i);
    }
    return 0;
}
