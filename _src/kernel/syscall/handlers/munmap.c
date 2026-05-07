#include <stdint.h>

#include "../sysc_handlers.h"
#include "kernel/mm.h"
#include "kernel/mm/uregion.h"
#include "kernel/task.h"
#include "lib/branch.h"
#include "lib/lock.h"


typedef enum {
    SYSC_MUNMAP_OK           = 0,
    SYSC_MUNMAP_ERR          = -1,
    SYSC_ALIGN_NOT_SUPPORTED = -2,
} sysc_munmap_res;

int64_t syscall64_unmap(
    sysarg_t                  addr,
    [[maybe_unused]] sysarg_t lenght,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5)
{
    if (unlikely(addr > KERNEL_BASE))
        return SYSC_MUNMAP_ERR;



    return SYSC_MUNMAP_ERR;
}