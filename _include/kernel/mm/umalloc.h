#pragma once

#include <arm/mmu.h>
#include <kernel/task.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>


struct utask;


/// allocates a user region, returns the kernel va for that region if marked as
/// permanent
void* umalloc(
    struct utask* t,
    uintptr_t usr_va,
    uint32_t pages,
    bool read,
    bool write,
    bool execute,
    // permanent for all the task life. The pa will automatically
    // be assigned without need for waiting for data aborts
    bool static_lifetime);

void* umalloc_assign_pa(struct utask* t, uintptr_t usr_va, uint32_t pages);


void ufree(struct utask* t, uintptr_t usr_va);


bool uregion_is_allocated(
    struct utask* t,
    uintptr_t start,
    size_t size,
    task_region** region);


bool uregion_is_assigned(
    struct utask* t,
    uintptr_t start,
    size_t size,
    task_region** out_region);
