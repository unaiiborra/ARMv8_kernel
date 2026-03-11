#include <stdint.h>
#include <syscall.h>

typedef enum {
    SYSC_PRINT = 0,
    SYSC_MAP = 1,

    SYSC_COUNT,
} syscall;


extern int64 _syscall(
    uint64 arg0,
    uint64 arg1,
    uint64 arg2,
    uint64 arg3,
    uint64 arg4,
    uint64 arg5,
    syscall sysc);


sysc_print_results syscall_print(const void* buf, size_t size)
{
    int64 result = _syscall((uint64)buf, size, 0, 0, 0, 0, SYSC_PRINT);

    if (result != SYSC_PRINT_OK && result != SYSC_PRINT_INVALID_BUF) {
        // TODO: syscall_exit()
    }

    return result;
}


void* syscall_map(size_t pages)
{
    if (pages == 0)
        return (void*)0;

    int64 result = _syscall(pages, 0, 0, 0, 0, 0, SYSC_MAP);

    if (result < 0) {
        // TODO: syscall_exit()
    }

    return (void*)result;
}
