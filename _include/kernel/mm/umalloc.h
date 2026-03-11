#pragma once

#include <arm/mmu.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <lib/stdint.h>

struct utask;


typedef struct {
    uint32 pages;
    uint32 flags;

    v_uintptr usr_start;
    v_uintptr knl_start; // zero if no pa has been assigned

    bitfield64 assigned_pa; // max of 64 pages
} usr_region_small;

typedef struct {
    uint32 pages;
    uint32 flags;

    v_uintptr usr_start;
    v_uintptr knl_start; // zero if no pa has been assigned

    bitfield64* pt_assigned_pa; // [ceil(pages / 64)]
} usr_region_big;


typedef union {
    struct {
        // pages determines whether a region is small or big depending if
        // it has more than 64 pages
        uint32 pages;
        uint32 flags;
        v_uintptr usr_start;
        v_uintptr knl_start;
        uint64 _;
    } any;

    usr_region_small sm;
    usr_region_big bg;
} usr_region;


/// allocates a user region, returns the kernel va for that region if marked as
/// permanent
void* umalloc(
    struct utask* t,
    uintptr usr_va,
    uint32 pages,
    bool read,
    bool write,
    bool execute,
    // permanent for all the task life. The pa will automatically
    // be assigned without need for waiting for data aborts
    bool permanent);

void* umalloc_assign_pa(struct utask* t, uintptr usr_va, uint32 pages);


void ufree(struct utask* t, uintptr usr_va);
