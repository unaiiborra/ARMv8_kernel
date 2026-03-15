#include "mm_mmu.h"

#include <arm/mmu.h>
#include <kernel/hardware.h>
#include <kernel/mm/mmu.h>
#include <lib/stdmacros.h>
#include <stddef.h>
#include <stdint.h>

#include "../init/mem_regions/early_kalloc.h"
#include "../malloc/internal/reserve_malloc.h"
#include "arm/sysregs/sysregs.h"
#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/mem.h"

mmu_mapping KERNEL_MAPPING;
mmu_mapping UNMAPPED_LO;
mmu_mapping* const MM_MMU_KERNEL_MAPPING = &KERNEL_MAPPING;
mmu_mapping* const MM_MMU_UNMAPPED_LO = &UNMAPPED_LO;


typedef struct {
    // for avoiding false sharing
    _Alignas(64) mmu_core_handle handle;
} local_mmu_core_handle;

static local_mmu_core_handle handles[NUM_CORES];


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


static void* unmapped_lo_allocator_first_tbl(size_t _)
{
    (void)_;
    pv_ptr pv = early_kalloc(
        MMU_GRANULARITY_4KB,
        "MM_MMU_UNMAPPED_LO table",
        true,
        false);

    return (void*)pv.va;
}


static void* unmapped_lo_allocator(size_t _)
{
    (void)_;
    PANIC("MM_MMU_UNMAPPED_LO should allways stay unmapped");
}


void mm_mmu_early_init()
{
    UNMAPPED_LO = mmu_mapping_new(
        MMU_LO,
        MMU_GRANULARITY_4KB,
        48,
        KERNEL_BASE,
        unmapped_lo_allocator_first_tbl,
        NULL);

    mmu_mapping_set_allocator(&UNMAPPED_LO, unmapped_lo_allocator);
}


mmu_core_handle* mm_mmu_core_handler_get(uint32_t coreid)
{
    if (coreid >= NUM_CORES)
        return NULL;

    return &handles[coreid].handle;
}


mmu_core_handle* mm_mmu_core_handler_get_self()
{
    uint64_t MPIDR_EL1 = sysreg_read(mpidr_el1);

    uint32_t mpidr_aff =
        ((MPIDR_EL1 >> 0) & 0xFF) | ((MPIDR_EL1 >> 8) & 0xFF) << 8 |
        ((MPIDR_EL1 >> 16) & 0xFF) << 16 | ((MPIDR_EL1 >> 32) & 0xFF) << 24;

    DEBUG_ASSERT(mpidr_aff < NUM_CORES);

    return &handles[mpidr_aff].handle;
}
