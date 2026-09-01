#define BUFFER_SIZE 4096
#define FLAG_READY 4

int flags(int f)
{
    return f & FLAG_READY;
}
