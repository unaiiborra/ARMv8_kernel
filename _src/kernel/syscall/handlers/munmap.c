#include <stddef.h>
#include <stdint.h>

#include "../sysc_handlers.h"
#include "arm/mmu.h"
#include "kernel/io/stdio.h"
#include "kernel/mm.h"
#include "kernel/mm/mmu.h"
#include "kernel/mm/page_malloc.h"
#include "kernel/mm/uregion.h"
#include "kernel/mm/vmalloc.h"
#include "kernel/panic.h"
#include "kernel/task.h"
#include "lib/align.h"
#include "lib/branch.h"
#include "lib/lock.h"
#include "lib/mem.h"


typedef enum {
    SYSC_MUNMAP_OK                  = 0,
    SYSC_MUNMAP_ERR                 = -1,
    SYSC_MUNMAP_ALIGN_NOT_SUPPORTED = -2,
} sysc_munmap_res;

int64_t syscall64_unmap(
    sysarg_t                  addr,
    sysarg_t                  lenght,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5)
{
    if (unlikely(addr > KERNEL_BASE)) {
        dbg_sysc_print(SYSC_MUNMAP, "SYSC_MUNMAP_ERR (addr > KERNEL_BASE)");
        return SYSC_MUNMAP_ERR;
    }

    if (unlikely(lenght == 0)) {
        dbg_sysc_print(SYSC_MUNMAP, "SYSC_MUNMAP_ERR (lenght == 0)");
        return SYSC_MUNMAP_ERR;
    }

    if (unlikely(!is_aligned(addr, PAGE_ALIGN) || lenght % PAGE_SIZE != 0)) {
        dbg_sysc_print(SYSC_MUNMAP, "SYSC_MUNMAP_ALIGN_NOT_SUPPORTED");
        return SYSC_MUNMAP_ALIGN_NOT_SUPPORTED;
    }

    task_t* owner = get_current_thread()->owner;
    size_t  pages = lenght / PAGE_SIZE;
    bool    freed;


    irqflags_t irqflags = spinlock_acquire_irqsave(&owner->lock);
    {
        freed = uregion_free(owner, addr, pages);
    }
    spinlock_release_irqrestore(&owner->lock, irqflags);


    if (unlikely(!freed)) {
        dbg_sysc_print(
            SYSC_MUNMAP,
            "SYSC_MUNMAP_ERR (uregion_free() returned false)");
        return SYSC_MUNMAP_ERR;
    }

    dbg_sysc_print(
        SYSC_MUNMAP,
        "SYSC_MUNMAP_OK (freed %p %d pages)",
        (void*)addr,
        (uint32_t)pages);

    return SYSC_MUNMAP_OK;
}