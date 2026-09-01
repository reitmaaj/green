/* green: lint=pass matrix=pass format=pass */

typedef unsigned long j89_len;

#define J89_BAD ((j89_len) - 1)

j89_len f(void);

j89_len f(void)
{
    j89_len r;

    r = J89_BAD;
    return r;
}
