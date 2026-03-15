#include <stddef.h>
#include <stdint.h>
#include <syscall.h>

typedef enum {
    SYSC_PRINT = 0,
    SYSC_MAP = 1,

    SYSC_COUNT,
} syscall;


extern int64 _syscall(
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    syscall sysc);


sysc_print_results syscall_print(const void* buf, size_t size)
{
    return _syscall((uint64_t)buf, size, 0, 0, 0, 0, SYSC_PRINT);
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
