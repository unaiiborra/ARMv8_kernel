#include "mm_mmu.h"

#include <arm/mmu.h>
#include <kernel/hardware.h>
#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/panic.h>
#include <kernel/smp.h>
#include <lib/mem.h>
#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <stddef.h>

#include "../init/mem_regions/early_kalloc.h"
#include "../malloc/internal/reserve_malloc.h"


static void*       mm_mmu_default_allocator(size_t bytes);
static mmu_mapping KERNEL_MAPPING;
static mmu_mapping UNMAPPED_LO;

mmu_mapping* const       MM_MMU_KERNEL_MAPPING     = &KERNEL_MAPPING;
mmu_mapping* const       MM_MMU_UNMAPPED_LO        = &UNMAPPED_LO;
const mmu_allocator      MM_STD_MMU_ALLOCATOR      = mm_mmu_default_allocator;
const mmu_allocator_free MM_STD_MMU_ALLOCATOR_FREE = raw_kfree;


typedef struct {
    // for avoiding false sharing
    _Alignas(64) mmu_core_handle handle;
} local_mmu_core_handle;

static local_mmu_core_handle handles[NUM_CPUS];


static void* mm_mmu_default_allocator(size_t bytes)
{
    (void)bytes;
    DEBUG_ASSERT(bytes == PAGE_SIZE);

    pv_ptr pv = reserve_malloc("mmu table");

    memzero64((void*)pv.va, bytes);

    DEBUG_ASSERT(pv.pa % PAGE_SIZE == 0);

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

    memzero64(
        (void*)pv.pa, // memzero from pa because mmu is not enabled yet
        _);

    return (void*)pv.va;
}


static void* unmapped_lo_allocator(size_t)
{
    PANIC("MM_MMU_UNMAPPED_LO should allways stay unmapped");
}


safe_early void mm_mmu_early_init()
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


mmu_core_handle* mm_mmu_core_handler_get(cpuid_t cpuid)
{
    if (cpuid >= NUM_CPUS)
        return NULL;

    return &handles[cpuid].handle;
}


mmu_core_handle* mm_mmu_core_handler_get_self()
{
    return &handles[get_cpuid()].handle;
}
