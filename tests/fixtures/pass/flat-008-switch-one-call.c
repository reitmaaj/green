/* green: lint=pass matrix=pass format=pass */

void handle(int n);

int run(int n);

int run(int n)
{
    switch (n)
    {
    case 1:
        handle(1);
        break;
    case 2:
        handle(2);
        break;
    default:
        handle(0);
        break;
    }
    return 0;
}
