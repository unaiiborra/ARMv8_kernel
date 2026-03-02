#pragma once

#include <lib/mem.h>
#include <lib/stdint.h>
#include <kernel/mm/page_malloc.h>

#include "../init/mem_regions/early_kalloc.h"

typedef uint64 page_mdt_bf;


void page_allocator_init();

/// allocates the early stage memory regions.
/// checking
void page_allocator_update_memregs(const early_memreg *mregs, size_t n);
void page_allocator_debug();
