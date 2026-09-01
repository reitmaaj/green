/* green: lint=green-preprocessor */

#define CAT(a, b) a##b

int CAT(my, _var);

int f(void)
{
    int r;

    r = my_var;
    return r;
}
