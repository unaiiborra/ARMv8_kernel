#include "../sysc_handlers.h"

int64_t syscall64_close(
    sysarg_t        fd,
    unused_sysarg_t a1,
    unused_sysarg_t a2,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5)
{
    (void)fd;
    PANIC("TODO: syscall64_close");
}
