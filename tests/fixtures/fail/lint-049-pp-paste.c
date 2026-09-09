/* green: lint=green-preprocessor msg="function-like macros are forbidden" */

#define CAT(a, b) a##b

int CAT(my, _var);

int f(void)
{
    int r;

    r = my_var;
    return r;
}
