#pragma once

#include <arm/mmu.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <lib/lock.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>

struct thread;

typedef struct uregion uregion_t;


// #define TASK_REGION_TRANSLATE_ERR 0xAAAA000000000000ul

// typedef enum {
//     KNL_TO_REG,
//     REG_TO_KNL,
// } tr_translate_opt;


// static inline uintptr_t task_region_translate(
//     const task_region* r,
//     uintptr_t          address,
//     tr_translate_opt   opt)
// {
//     if (!r)
//         return TASK_REGION_TRANSLATE_ERR;

//     if (!r->knl_start)
//         return TASK_REGION_TRANSLATE_ERR;

//     uintptr_t base = (opt == REG_TO_KNL) ? r->reg_start : r->knl_start;
//     uintptr_t size = (uintptr_t)r->pages * PAGE_SIZE;

//     if (address < base)
//         return TASK_REGION_TRANSLATE_ERR;

//     uintptr_t offset = address - base;

//     if (offset >= size)
//         return TASK_REGION_TRANSLATE_ERR;

//     uintptr_t result_address = (opt == REG_TO_KNL) ? r->knl_start + offset
//                                                    : r->reg_start + offset;

//     if (!address_is_valid(result_address, KERNEL_ADDR_BITS, true))
//         return TASK_REGION_TRANSLATE_ERR;

//     return result_address;
// }




typedef enum {
    TASK_NEW, // just created, might not even have threads assigned

    TASK_ALIVE, // has at least 1 thread alive

    TASK_DYING, // the process called exit or aborted, defers the killing of
                // threads
    TASK_DEAD,  // task has no threads alive
} task_state;


typedef struct {
    uint64_t           task_uid;
    const char*        name;
    spinlock_t         lock;
    _Atomic task_state state;
    uint32_t           stack_pages;
    mmu_mapping        mapping;
    uregion_t*         regions;
    kvec(thread*) threads;
} task_t;


task_t* task_new(const char* name, size_t stack_size);
void    terminate_task(uint32_t exit_code);
