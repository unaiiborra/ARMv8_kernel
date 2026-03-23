#pragma once

#include <kernel/mm.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <stdint.h>


typedef struct {
    uint32_t pages;
    uint32_t flags;

    vuintptr_t reg_start; // usr_start in case of a user task
    vuintptr_t knl_start; // zero if no pa has been assigned

    union {
        bitfield64 sm;  // max of 64 pages
        bitfield64* bg; // more than 64 possible pages (array of bitfield64)
    } assigned_pa;
} task_region;


#define TASK_REGION_TRANSLATE_ERR 0xAAAA000000000000ul

typedef enum {
    KNL_TO_REG,
    REG_TO_KNL,
} task_region_translate_opt;


static inline uintptr_t task_region_translate(
    const task_region* r,
    uintptr_t address,
    task_region_translate_opt opt)
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

    return (opt == REG_TO_KNL) ? r->knl_start + offset : r->reg_start + offset;
}