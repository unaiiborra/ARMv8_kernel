#pragma once

#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>
/*
 *  As I dont have time to develop a dtb parser, i hardcode the info in this
 * file. It is the memory map of the SoC
 */

typedef enum {
    MEM_REGION_RESERVED = -1,
    MEM_REGION_DDR,
    MEM_REGION_MMIO,
} mem_regions_type;


typedef struct {
    const char* tag;
    p_uintptr_t start;
    size_t size;
    mem_regions_type type;
} mem_region;


typedef struct mem_regions {
    const size_t REG_COUNT;
    const mem_region* const REGIONS;
} mem_regions;


extern const mem_regions MEM_REGIONS;
extern const mem_regions MEM_REGIONS_RESERVED;
