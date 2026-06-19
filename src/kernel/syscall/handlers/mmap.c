#include <kernel/mm.h>
#include <kernel/mm/uregion.h>
#include <kernel/panic.h>
#include <lib/branch.h>
#include <lib/lock.h>
#include <lib/math.h>
#include <stddef.h>
#include <stdint.h>

#include "../sysc_handlers.h"
#include "lib/align.h"


typedef enum {
    // SYSC_MMAP_OK >= 0, (address)
    SYSC_MMAP_ERR               = -1,
    SYSC_MMAP_CFG_NOT_SUPPORTED = -2,
} sysc_mmap_res;


#define PROT_NONE 0

typedef enum {
    // bit positions
    PROT_READ  = 1 << 0,
    PROT_WRITE = 1 << 1,
    PROT_EXEC  = 1 << 2,
} sysc_mmap_prot;


typedef enum {
    MAP_ANONYMOUS = 1 << 0, // map not supported by a file
    MAP_FIXED     = 1 << 1, // the provided address is not a suggestion, fail if
                            // overlap
    // TODO: implement file system
} sysc_mmap_flags;


int64_t syscall64_mmap(
    sysarg_t                  addr,
    sysarg_t                  lenght,
    sysarg_t                  prot,
    sysarg_t                  flags,
    unused_sysarg_t fd,
    unused_sysarg_t offset)
{
    task_t* owner = get_current_thread()->owner;


    if (unlikely(!(flags & MAP_ANONYMOUS))) {
        dbg_sysc_print(SYSC_MMAP, "SYSC_MMAP_ERR (!MAP_ANONYMOUS)");
        return SYSC_MMAP_ERR;
    }

    if (unlikely(
            addr != (sysarg_t)NULL && (flags & MAP_FIXED) &&
            (addr % PAGE_ALIGN != 0))) {
        dbg_sysc_print(
            SYSC_MMAP,
            "SYSC_MMAP_ERR (addr != (sysarg_t)NULL && (flags & MAP_FIXED) && "
            "(addr %% PAGE_ALIGN != 0))");
        return SYSC_MMAP_ERR;
    }

    if (unlikely(
            prot == PROT_NONE ||
            ((prot & PROT_WRITE) &&
             (prot & PROT_EXEC)))) // wx is not supported by arm mmu
    {
        // TODO: implement PROT_NONE
        dbg_sysc_print(SYSC_MMAP, "SYSC_MMAP_CFG_NOT_SUPPORTED");
        return SYSC_MMAP_CFG_NOT_SUPPORTED;
    }

    if (unlikely(lenght == 0)) {
        dbg_sysc_print(SYSC_MMAP, "SYSC_MMAP_ERR (lenght == 0)");
        return SYSC_MMAP_ERR;
    }

    size_t pages = div_ceil(lenght, PAGE_SIZE);


    spinlocked(&owner->memory_lock)
    {
        uintptr_t address;

        if (addr == (sysarg_t)NULL) {
            bool found = uregion_find_free(owner, pages, &address);

            if (unlikely(!found))
                goto out_of_memory;
        }
        else if (addr % PAGE_ALIGN == 0)
            address = addr;
        else
            address = align_down(addr, PAGE_ALIGN);

        uregion_reserve_e ures = uregion_reserve(
            owner,
            address,
            pages,
            prot & PROT_READ,
            prot & PROT_WRITE,
            prot & PROT_EXEC);


        switch (expect(ures, UREGION_OK)) {
            case UREGION_OK:
                dbg_sysc_print(
                    SYSC_MMAP,
                    "UREGION_OK [%p %d pages]",
                    address,
                    pages);
                return address;

            case UREGION_OVERLAPS:
                DEBUG_ASSERT(addr != (sysarg_t)NULL);

                // the requested address couldn't be allocated (it overlaps
                // other pages) so the kernel will decide where to map the
                // pages

                if (flags & MAP_FIXED) {
                    dbg_sysc_print(SYSC_MMAP, "SYSC_MMAP_ERR (MAP_FIXED)");
                    return SYSC_MMAP_ERR;
                }

                bool found = uregion_find_free(owner, pages, &address);

                if (unlikely(!found))
                    goto out_of_memory;


                ures = uregion_reserve(
                    owner,
                    address,
                    div_ceil(lenght, PAGE_SIZE),
                    prot & PROT_READ,
                    prot & PROT_WRITE,
                    prot & PROT_EXEC);

                DEBUG_ASSERT(ures == UREGION_OK);

                dbg_sysc_print(
                    SYSC_MMAP,
                    "UREGION_OK [requested %p but was not available, provided "
                    "%p %d pages]",
                    addr,
                    address,
                    pages);

                return SYSC_MMAP_CFG_NOT_SUPPORTED;

            case UREGION_ERROR:
                dbg_sysc_print(
                    SYSC_MMAP,
                    "UREGION_ERROR (uregion_reserve error!)");

                return SYSC_MMAP_ERR;

            default:
                PANIC();
        }
    }

out_of_memory: {
    dbg_sysc_print(SYSC_MMAP, "SYSC_MMAP_ERR (out of memory)");
    return SYSC_MMAP_ERR;
}

    unreachable();
}
