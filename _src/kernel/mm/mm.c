#include "kernel/mm.h"

#include <arm/mmu.h>
#include <frdm_imx8mp.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mm_info.h"


extern uintptr_t _get_pc(void);


bool mm_kernel_is_relocated()
{
    uintptr_t pc = _get_pc();

    return ((pc >> 47) & 0x1FFFFUL) == 0x1FFFFUL;
}


void mm_dbg_print_mmu()
{
    //    mmu_debug_dump(&mm_mmu_h, MMU_TBL_LO);
    //  mmu_debug_dump(&mm_mmu_h, MMU_TBL_HI);
}


bool mm_va_is_in_kmap_range(void* ptr)
{
    if (!is_kva(ptr))
        return false;

    if ((v_uintptr_t)ptr < kpa_to_kva(mm_info_mm_addr_space()))
        return true;

    return false;
}
