#pragma once

#include <arm/mmu.h>
#include <kernel/task.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>




/// allocates a user region, returns the kernel va for that region if marked as
/// permanent
void* umalloc(
    task*     t,
    uintptr_t reg_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      execute,
    // permanent for all the task life. The pa will automatically
    // be assigned without need for waiting for data aborts
    bool static_lifetime);

void* umalloc_assign_pa(task* t, uintptr_t usr_va, uint32_t pages);


void ufree(task* t, uintptr_t usr_va);


bool uregion_is_mapped(
    task*         t,
    uintptr_t     start,
    size_t        size,
    task_region** out_region);


bool uregion_is_assigned(
    task*         t,
    uintptr_t     start,
    size_t        size,
    task_region** out_region);



uintptr_t tregion_find_free(task* t, size_t pages);


typedef enum {
    UMALLOC_F_READ             = 0,
    UMALLOC_F_WRITE            = 1,
    UMALLOC_F_EXEC             = 2,
    UMALLOC_F_STATIC_LIFETIME  = 3,
    UMALLOC_F_FULL_MAPPED      = 4,
    UMALLOC_F_PARTIALLY_MAPPED = 5,
} umalloc_flags;


static inline void umalloc_set_flag(task_region* treg, uint32_t flag, bool val)
{
    treg->flags =
        (treg->flags & ~(1U << flag)) | ((unsigned int)(!!val) << flag);
}

static inline bool umalloc_get_flag(task_region* treg, uint32_t flag)
{
    return (treg->flags >> flag) & 1U;
}