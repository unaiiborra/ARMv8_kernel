#include "../sysc_handlers.h"

int64_t syscall64_open(
    sysarg_t        path,
    sysarg_t        flags,
    sysarg_t        mode,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5)
{
    (void)path, (void)flags, (void)mode;
    PANIC("TODO: syscall64_open");
}
