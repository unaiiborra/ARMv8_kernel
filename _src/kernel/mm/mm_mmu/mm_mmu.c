#include "mm_mmu.h"

#include <arm/mmu.h>
#include <kernel/hardware.h>
#include <lib/stdmacros.h>

#include "../init/mem_regions/early_kalloc.h"
#include "../malloc/internal/reserve_malloc.h"
#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/mem.h"
#include "lib/stdint.h"

mmu_mapping MM_MMU_KERNEL_MAPPING;
mmu_mapping MM_MMU_UNMAPPED_LO;


static mmu_core_handle handles[NUM_CORES];


static void* mm_mmu_default_allocator(size_t bytes)
{
    (void)bytes;
    DEBUG_ASSERT(bytes == KPAGE_SIZE);

    pv_ptr pv = reserve_malloc("mmu table");

    DEBUG_ASSERT(pv.pa % KPAGE_SIZE == 0);

    return (void*)pv.va;
}


mmu_mapping mm_mmu_mapping_new(mmu_tbl_rng rng)
{
    return mmu_mapping_new(
        rng,
        MMU_GRANULARITY_4KB,
        KERNEL_ADDR_BITS,
        KERNEL_BASE,
        mm_mmu_default_allocator,
        raw_kfree);
}


static void* unmapped_lo_allocator_first_tbl(size_t)
{
    pv_ptr pv = early_kalloc(
        MMU_GRANULARITY_4KB,
        "MM_MMU_UNMAPPED_LO table",
        true,
        false);

    return (void*)pv.va;
}

static void* unmapped_lo_allocator(size_t)
{
    PANIC("MM_MMU_UNMAPPED_LO should allways stay unmapped");
}

void mm_mmu_early_init()
{
    MM_MMU_UNMAPPED_LO = mmu_mapping_new(
        MMU_LO,
        MMU_GRANULARITY_4KB,
        48,
        KERNEL_BASE,
        unmapped_lo_allocator_first_tbl,
        NULL);

    mmu_mapping_set_allocator(&MM_MMU_UNMAPPED_LO, unmapped_lo_allocator);
}


mmu_core_handle* mm_mmu_core_handler_get(uint32 coreid)
{
    if (coreid >= NUM_CORES)
        return NULL;

    return &handles[coreid];
}


mmu_core_handle* mm_mmu_core_handler_get_self()
{
    uint64 MPIDR_EL1;

    asm volatile("mrs %0, mpidr_el1" : "=r"(MPIDR_EL1) : : "memory");

    uint32 mpidr_aff =
        ((MPIDR_EL1 >> 0) & 0xFF) | ((MPIDR_EL1 >> 8) & 0xFF) << 8 |
        ((MPIDR_EL1 >> 16) & 0xFF) << 16 | ((MPIDR_EL1 >> 32) & 0xFF) << 24;

    DEBUG_ASSERT(mpidr_aff < NUM_CORES);

    return &handles[mpidr_aff];
}
