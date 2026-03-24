#pragma once

#include <kernel/mm.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/mmu.h"
#include "kernel/lib/kvec.h"
#include "lib/lock.h"

typedef struct {
    uint32_t pages;
    uint32_t flags;

    vuintptr_t reg_start; // usr_start in case of a user task
    vuintptr_t knl_start; // zero if no pa has been assigned

    union {
        // (pages <= 64) max of 64 pages
        bitfield64 sm;

        // (pages > 64)more than 64 possible pages (array of bitfield64)
        bitfield64* bg;
    } assigned_pa;
} task_region;

#define TASK_REGION_TRANSLATE_ERR 0xAAAA000000000000ul

typedef enum {
    KNL_TO_REG,
    REG_TO_KNL,
} tr_translate_opt;


static inline uintptr_t task_region_translate(
    const task_region* r,
    uintptr_t          address,
    tr_translate_opt   opt)
{
    if (!r)
        return TASK_REGION_TRANSLATE_ERR;

    if (!r->knl_start)
        return TASK_REGION_TRANSLATE_ERR;

    uintptr_t base = (opt == REG_TO_KNL) ? r->reg_start : r->knl_start;
    uintptr_t size = (uintptr_t)r->pages * KPAGE_SIZE;

    if (address < base)
        return TASK_REGION_TRANSLATE_ERR;

    uintptr_t offset = address - base;

    if (offset >= size)
        return TASK_REGION_TRANSLATE_ERR;

    uintptr_t result_address =
        (opt == REG_TO_KNL) ? r->knl_start + offset : r->reg_start + offset;

    if (!address_is_valid(result_address, KERNEL_ADDR_BITS, true))
        return TASK_REGION_TRANSLATE_ERR;

    return result_address;
}




typedef enum {
    TASK_NEW, // just created, might not even have threads assigned

    TASK_ALIVE, // has at least 1 thread alive

    TASK_DYING, // the process called exit or aborted, defers the killing of
                // threads
    TASK_DEAD,  // task has no threads alive
} task_state;

typedef struct {
    uint64_t     task_uid;
    const char*  name;
    spinlock_t   lock;
    task_state   state;
    uint32_t     stack_pages;
    mmu_mapping  mapping;
    task_region* regions;
    kvec_T(thread*) threads;
} task;


task* task_new(const char* name, size_t stack_size);
void  task_delete(task* t);