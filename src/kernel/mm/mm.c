#include "kernel/mm.h"

#include <arm/mmu.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mm_info.h"


extern uintptr_t _get_pc(void);


void mm_dbg_print_mmu()
{
    //    mmu_debug_dump(&mm_mmu_h, MMU_TBL_LO);
    //  mmu_debug_dump(&mm_mmu_h, MMU_TBL_HI);
}


bool mm_va_is_in_kmap_range(void* ptr)
{
    if (!is_kva(ptr))
        return false;

    if ((vuintptr_t)ptr < kpa_to_kva(mm_info_mm_addr_space()))
        return true;

    return false;
}
