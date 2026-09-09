/* green: lint=green-declaration msg="one declaration must declare exactly one object" */

int f(void)
{
    int a, b;
    int c;

    a = 1;
    b = 2;
    c = 3;
    return a + b + c;
}
