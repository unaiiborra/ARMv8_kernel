#include <arm/exceptions/ctx.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <stddef.h>
#include <stdint.h>

#include "lib/lock.h"
#include "sysc_handlers.h"

static const syscall_handler SYSC64_TABLE[SYSC_COUNT] = {
    [SYSC_EXIT]   = syscall64_exit,
    [SYSC_OPEN]   = syscall64_open,
    [SYSC_CLOSE]  = syscall64_close,
    [SYSC_READ]   = syscall64_read,
    [SYSC_WRITE]  = syscall64_write,
    [SYSC_SPAWN]  = syscall64_spawn,
    [SYSC_KILL]   = syscall64_kill,
    [SYSC_YIELD]  = syscall64_yield,
    [SYSC_MMAP]   = syscall64_mmap,
    [SYSC_MUNMAP] = syscall64_munmap,
};

static inline void sysc64_set_result(gpr_t* gpr, int64_t result)
{
    gpr[SYSC64_RETURN_REG] = result;
}

void sysc64_dispatch()
{
    irqlocked()
    {
        gpr_t* gpr = get_current_thread()->ctx.x;

        size_t syscall_id = gpr[SYSC64_SYSCNUM_REG];

        if (syscall_id >= SYSC_COUNT) {
            sysc64_set_result(gpr, SYSC64_RES_UNKNOWN);
            return;
        }

        int64_t result = SYSC64_TABLE[syscall_id](
            gpr[0],
            gpr[1],
            gpr[2],
            gpr[3],
            gpr[4],
            gpr[5]);

        sysc64_set_result(gpr, result);
    }
}
