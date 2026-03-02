#pragma once

#include <arm/mmu.h>

#define NUM_CORES         4
#define MM_MMU_LO_BITS    48
#define MM_MMU_HI_BITS    48

extern mmu_mapping MM_MMU_KERNEL_MAPPING;

extern mmu_mapping MM_MMU_UNMAPPED_LO;


void mm_mmu_early_init();
mmu_core_handle * mm_mmu_core_handler_get(uint32 coreid);
mmu_core_handle * mm_mmu_core_handler_get_self();
