#pragma once

#include <kernel/mm/page_malloc.h>
#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>

#include "../init/mem_regions/early_kalloc.h"

typedef uint64_t page_mdt_bf;


void page_allocator_init();

/// allocates the early stage memory regions.
/// checking
void page_allocator_update_memregs(const early_memreg* mregs, size_t n);
void page_allocator_debug();
