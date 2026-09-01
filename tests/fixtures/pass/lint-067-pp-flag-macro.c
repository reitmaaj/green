/* green: lint=pass */

#define FLAG_READY 4
#define FLAG_DONE 8

int flags(int f)
{
    return f & FLAG_READY;
}
