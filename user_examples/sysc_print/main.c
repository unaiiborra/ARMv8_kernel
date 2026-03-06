

typedef signed long int64;
typedef unsigned long uint64;


extern int64 _supervisor_call(
    uint64 arg0,
    uint64 arg1,
    uint64 arg2,
    uint64 arg3,
    uint64 arg4,
    uint64 arg5);

int main()
{
    char test[26] = "\n\rHello from userspace!\n\r";

    __attribute((unused)) int64 result =
        _supervisor_call((uint64)&test[0], 26, 0, 0, 0, 0);

    while (1) {
    }
}