#include "reloc.h"

#include <arm/mmu.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <lib/stdmacros.h>
#include <lib/unit/mem.h>
#include <stdbool.h>

#include "../init/identity_mapping.h"
#include "../init/mem_regions/early_kalloc.h"
#include "kernel/mm/mmu.h"

extern noreturn void _jmp_to_with_offset(void* to, size_t offset);
extern noreturn void _reloc_cfg_end(void);


void mm_reloc_kernel()
{
    ASSERT(mmu_is_active());


    // jumps to the asm fn _reloc_cfg_end (relocated with KERNEL BASE).
    // _reloc_cfg_end returns to reloc_cfg_end
    _jmp_to_with_offset(_reloc_cfg_end, KERNEL_BASE);
}


void reloc_cfg_end()
{
    mmu_mapping* const EARLY_MMU_MAPPINGS[2] = {
        &early_lo_mapping,
        MM_MMU_KERNEL_MAPPING,
    };

    const mmu_mapping MMU_DEFAULT_KERNEL_MAPPING = mm_mmu_mapping_new(MMU_HI);

    for (size_t i = 0; i < ARRAY_LEN(EARLY_MMU_MAPPINGS); i++) {
        mmu_mapping_set_allocator(
            EARLY_MMU_MAPPINGS[i],
            mmu_mapping_get_allocator(&MMU_DEFAULT_KERNEL_MAPPING));

        mmu_mapping_set_allocator_free(
            EARLY_MMU_MAPPINGS[i],
            mmu_mapping_get_allocator_free(&MMU_DEFAULT_KERNEL_MAPPING));

        mmu_mapping_set_physmap_offset(EARLY_MMU_MAPPINGS[i], KERNEL_BASE);
    }


    // get first free heap va
    early_memreg* mblcks;
    size_t n;

    early_kalloc_get_memregs(&mblcks, &n);
    vuintptr_t free_heap_start =
        kpa_to_kva(mblcks[n - 1].addr + (mblcks[n - 1].pages * KPAGE_SIZE));


    mmu_core_handle* ch0 = mm_mmu_core_handler_get_self();

    mmu_mapping* LO_MAPPING = mmu_core_get_lo_mapping(ch0);
    mmu_mapping* HI_MAPPING = mmu_core_get_hi_mapping(ch0);

    // relocate the table pointers to the va
    UNSAFE_mmu_mapping_set_tbl_address(LO_MAPPING, pt_as_kva(LO_MAPPING->tbl_));

    UNSAFE_mmu_mapping_set_tbl_address(HI_MAPPING, pt_as_kva(HI_MAPPING->tbl_));

    mmu_core_set_mapping(ch0, pt_as_kva(LO_MAPPING));
    mmu_core_set_mapping(ch0, pt_as_kva(HI_MAPPING));

    // unmap the identity mapping
    bool result = mmu_core_set_mapping(ch0, MM_MMU_UNMAPPED_LO);
    ASSERT(result);

    mmu_delete_mapping(&early_lo_mapping);

    mmu_unmap(
        MM_MMU_KERNEL_MAPPING,
        free_heap_start,
        MEM_TiB,
        NULL); // TODO: unmap from the actually reserved memory


    extern noreturn void _return_to_kernel_entry(size_t physmap_offset);
    _return_to_kernel_entry(KERNEL_BASE);
}
