/* green: matrix=pass */

int low(unsigned int v);

int low(unsigned int v)
{
    return (int)(v & 1u);
}
