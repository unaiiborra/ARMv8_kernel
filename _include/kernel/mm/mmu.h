#pragma once

#include <arm/mmu.h>

#define MM_MMU_LO_BITS 48
#define MM_MMU_HI_BITS 48

extern mmu_mapping* const MM_MMU_KERNEL_MAPPING;
extern mmu_mapping* const MM_MMU_UNMAPPED_LO;

mmu_core_handle* mm_mmu_core_handler_get(uint32_t coreid);
mmu_core_handle* mm_mmu_core_handler_get_self();
