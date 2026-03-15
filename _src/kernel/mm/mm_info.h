#pragma once


#include <stddef.h>

#include "lib/mem.h"


typedef struct {
    vuintptr_t start;
    vuintptr_t end;
    size_t size;
} mm_ksection;

typedef struct {
    mm_ksection text;
    mm_ksection rodata;
    mm_ksection data;
    mm_ksection bss;
    mm_ksection stacks;
} mm_ksections;


void mm_info_init();


size_t mm_info_ddr_size(void);

uintptr_t mm_info_ddr_start(void);
uintptr_t mm_info_ddr_end(void);

uintptr_t mm_info_kernel_start(void);

size_t mm_info_page_count(void);

size_t mm_info_mm_addr_space(void);


extern const mm_ksections MM_KSECTIONS;
