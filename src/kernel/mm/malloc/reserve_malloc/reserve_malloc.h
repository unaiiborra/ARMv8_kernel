#pragma once

#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>
/*
 *  Reserve allocator that reserves allocated pages (that must be mapped with
 * KERNEL_BASE relation). The main use case is for when the raw_allocator needs
 * a page for its internal components, as it cannot reallocate itself, it
 * requests the reserve to give him a prereserved page. After finishing the
 * initial allocation, it must allocate new pages for refilling the reserve.
 */


extern const size_t RESERVE_MALLOC_RESERVE_SIZE;


void reserve_malloc_init();

pv_ptr reserve_malloc(const char* new_tag);

void reserve_malloc_fill();
