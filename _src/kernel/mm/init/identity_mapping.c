#include "identity_mapping.h"

#include <arm/mmu.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "../init/mem_regions/early_kalloc.h"
#include "../mm_mmu/mm_mmu.h"
#include "mem_regions/mem_regions.h"


mmu_mapping early_lo_mapping;


/// returns the pa, not va, but as when relocating the address will be
/// relocated, works fine. Providing the va at this stage cannot work as it is
/// the allocator for initializing the first identity mapping without the mmu
/// still enabled
static void* im_alloc(size_t bytes)
{
    p_uintptr pa = early_kalloc(bytes, "mmu table", false, false).pa;

    memzero((void*)pa, bytes);

    DEBUG_ASSERT(pa % MMU_GRANULARITY_4KB == 0);

    return (void*)pa;
}


static void im_free(void* addr)
{
    char buf[200];

    stdint_to_ascii(
        (STDINT_UNION) {.uint64 = (v_uintptr)addr},
        STDINT_UINT64,
        buf,
        200,
        STDINT_BASE_REPR_HEX);

    kprintf("%s\n\r", buf);

#ifndef DEBUG
    PANIC("The early identity mapping allocations should not free any tables");
#endif
}


void early_identity_mapping()
{
    mmu_core_handle* core0_handle = mm_mmu_core_handler_get_self();

    early_lo_mapping = mmu_mapping_new(
        MMU_LO,
        MMU_GRANULARITY_4KB,
        48,
        0x0,
        (void*)mm_as_kpa((uintptr)im_alloc),
        (void*)mm_as_kpa((uintptr)im_free));

    MM_MMU_KERNEL_MAPPING = mmu_mapping_new(
        MMU_HI,
        MMU_GRANULARITY_4KB,
        48,
        0x0,
        (void*)mm_as_kpa((uintptr)im_alloc),
        (void*)mm_as_kpa((uintptr)im_free));


    const mmu_pg_cfg DEVICE_CFG = mmu_pg_cfg_new(
        1,
        MMU_AP_EL0_NONE_EL1_RW,
        MMU_SH_NON_SHAREABLE,
        false,
        1,
        0,
        1,
        0);

    const mmu_pg_cfg MEM_CFG = mmu_pg_cfg_new(
        0,
        MMU_AP_EL0_NONE_EL1_RW,
        MMU_SH_INNER_SHAREABLE,
        false,
        1,
        0,
        1,
        0);


    for (size_t i = 0; i < MEM_REGIONS.REG_COUNT; i++) {
        const mem_region r = mm_as_kpa_ptr(MEM_REGIONS.REGIONS)[i];

        const mmu_pg_cfg* CFG;
        switch (r.type) {
            case MEM_REGION_DDR:
                CFG = &MEM_CFG;
                break;
            case MEM_REGION_MMIO:
                CFG = &DEVICE_CFG;
                break;
            default:
                PANIC();
                break;
        }

        mmu_map_result mres;
        mres = mmu_map(
            &early_lo_mapping,
            mm_as_kpa(r.start),
            mm_as_kpa(r.start),
            r.size,
            *CFG,
            NULL);
        ASSERT(mres == MMU_MAP_OK);

        mres = mmu_map(
            &MM_MMU_KERNEL_MAPPING,
            mm_as_kva(r.start),
            mm_as_kpa(r.start),
            r.size,
            *CFG,
            NULL);
        ASSERT(mres == MMU_MAP_OK);
    }


    bool result = mmu_core_handle_new(
        core0_handle,
        &early_lo_mapping,
        &MM_MMU_KERNEL_MAPPING,
        true,
        true,
        true,
        true,
        false);
    ASSERT(result);


    mmu_activate_result cres = mmu_core_activate(core0_handle);
    ASSERT(cres == MMU_ACTIVATE_OK);
}
