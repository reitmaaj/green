/* green: lint=green-flat */

void a(void);

void b(void);

int run(int n);

int run(int n)
{
    int x;

    x = n;
    {
        a();
        b();
    }
    return x;
}
