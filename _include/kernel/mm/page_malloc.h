#pragma once

#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
    const char* tag;
    // TODO: use a bitfield
    bool device_mem;
    bool permanent;
    uint8_t cache_size; // saved as (log2(cache_size)).
} mm_page_data;


static inline mm_page_data
mm_page_data_new(const char* tag, bool device_mem, bool permanent)
{
    return (mm_page_data) {
        .tag = tag,
        .device_mem = device_mem,
        .permanent = permanent,
        .cache_size = 0,
    };
}

typedef struct {
    p_uintptr_t pa;
    mm_page_data data;
} mm_page;


p_uintptr_t page_malloc(uint8_t order, mm_page_data p);
void page_free(p_uintptr_t pa);
const char* page_allocator_update_tag(p_uintptr_t pa, const char* new_tag);

// copies the data from the page to the provided address
bool page_allocator_get_data(p_uintptr_t pa, mm_page_data* data);
bool page_allocator_set_data(p_uintptr_t pa, mm_page_data data);