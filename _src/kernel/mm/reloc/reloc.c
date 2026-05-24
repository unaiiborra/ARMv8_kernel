#include <arm/mmu.h>
#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <lib/unit/mem.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../init/mem_regions/early_kalloc.h"
#include "lib/math.h"

extern noreturn void _jmp_to_with_offset(
    puintptr_t to,
    size_t     offset,
    uint64_t   ctx);
extern noreturn void _reloc_cfg_end(void);

extern void _mov_sp_blr(void* stack_bottom, void* return_address);

safe_early void mm_reloc(void* va_ret_addr)
{
    ASSERT(mmu_is_active());

    // jumps to the asm fn _reloc_cfg_end (relocated with KERNEL BASE).
    // _reloc_cfg_end returns to reloc_cfg_end
    _jmp_to_with_offset(
        (puintptr_t)_reloc_cfg_end,
        KERNEL_BASE,
        (uint64_t)va_ret_addr);
}


void early_reloc_cfg_end()
{
    mmu_mapping* const EARLY_MMU_MAPPINGS[2] = {
        MM_MMU_IDENTITY_LO_MAPPING,
        MM_MMU_KERNEL_MAPPING,
    };


    // update allocaror and free fns as virtual addresses (done only in early
    // initialization)
    for (size_t i = 0; i < ARRAY_LEN(EARLY_MMU_MAPPINGS); i++) {
        mmu_mapping_set_allocator(
            EARLY_MMU_MAPPINGS[i],
            as_kva(MM_STD_MMU_ALLOCATOR));

        mmu_mapping_set_allocator_free(
            EARLY_MMU_MAPPINGS[i],
            as_kva(MM_STD_MMU_ALLOCATOR_FREE));

        mmu_mapping_set_physmap_offset(EARLY_MMU_MAPPINGS[i], KERNEL_BASE);
    }


    // get first free heap va
    early_memreg* mblcks;
    size_t        n;

    early_kalloc_get_memregs(&mblcks, &n);
    vuintptr_t free_heap_start = kpa_to_kva(
        mblcks[n - 1].addr + (mblcks[n - 1].pages * PAGE_SIZE));


    mmu_core_handle* ch = mm_mmu_core_handler_get_self();

    mmu_mapping* LO_MAPPING = mmu_core_get_lo_mapping(ch);
    mmu_mapping* HI_MAPPING = mmu_core_get_hi_mapping(ch);

    // relocate the table pointers to the va
    UNSAFE_mmu_mapping_set_tbl_address(LO_MAPPING, as_kva(LO_MAPPING->tbl_));

    UNSAFE_mmu_mapping_set_tbl_address(HI_MAPPING, as_kva(HI_MAPPING->tbl_));

    mmu_core_set_mapping(ch, as_kva(LO_MAPPING));
    mmu_core_set_mapping(ch, as_kva(HI_MAPPING));

    // unmap the identity mapping
    bool result = mmu_core_set_mapping(ch, MM_MMU_UNMAPPED_LO);
    ASSERT(result);


    mmu_unmap(
        MM_MMU_KERNEL_MAPPING,
        free_heap_start,
        MEM_TiB,
        NULL); // TODO: unmap from the actually reserved memory


    extern noreturn void kernel_entry();



    raw_kmalloc_cfg stack_cfg = {
        .fill_reserve = true,
        .assign_pa    = true,
        .kmap         = true,
        .device_mem   = false,
        .permanent    = true,
        .init_zeroed  = true,
    };

    const size_t STACK_SIZE = 4 * MEM_MiB;

    void* stack_top = raw_kmalloc(
        div_ceil(STACK_SIZE, PAGE_SIZE),
        "kernel stack region",
        &stack_cfg);

    void* stack_bottom = (char*)stack_top + STACK_SIZE;

    _mov_sp_blr(stack_bottom, kernel_entry);
}
