/* green: lint=green-flat */

void a(void);

void b(void);

int run(int n);

int run(int n)
{
    if (n > 0)
    {
        a();
        b();
    }
    return 0;
}
