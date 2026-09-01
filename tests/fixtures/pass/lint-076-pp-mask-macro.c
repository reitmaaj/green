/* green: lint=pass matrix=pass format=pass */

#define FLAG_A 1
#define FLAG_B 2

#define FLAG_MASK (FLAG_A | FLAG_B)

int flags(int f);

int flags(int f)
{
    return f & FLAG_MASK;
}
