#pragma once

#include <lib/mem.h>
#include <stddef.h>
/*
 *  As I dont have time to develop a dtb parser, i hardcode the info in this
 *  file. It is the memory map of the SoC
 */

typedef enum {
    // Reserved, not usable by the kernel, it can overlap with other regions.
    MEM_REGION_RESERVED = -1,
    // Usable by the kernel as DDR memory, it can not overlap with other regions.
    MEM_REGION_DDR,
    // Usable by the kernel as MMIO memory, it can not overlap with other regions.
    MEM_REGION_MMIO,
} mem_regions_type;


typedef struct {
    const char* tag;
    puintptr_t start;
    size_t size;
    mem_regions_type type;
} mem_region;


typedef struct mem_regions {
    const size_t REG_COUNT;
    const mem_region* const REGIONS;
} mem_regions;

extern const mem_regions MEM_REGIONS;
