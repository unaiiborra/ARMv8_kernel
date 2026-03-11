#pragma once

#include <kernel/mm.h>

#include "../../phys/page_allocator.h"

#define MIN_CACHE CACHE_8
#define MAX_CACHE CACHE_1024


#define CACHE_MALLOC_SUPPORTED_SIZES 8

// pages per cache
#define STATIC_ASSERT_POW2(N)                                                  \
    _Static_assert(                                                            \
        ((N) & ((N) - 1)) == 0,                                                \
        "cache page count must be of order 2 as page_free relies on aligning " \
        "down to "                                                             \
        "the cache size and needs the va to be aligned to the size")
#define CACHE_8_PAGES 4
#define CACHE_16_PAGES 4
#define CACHE_32_PAGES 4
#define CACHE_64_PAGES 4
#define CACHE_128_PAGES 4
#define CACHE_256_PAGES 4
#define CACHE_512_PAGES 4
#define CACHE_1024_PAGES 8
STATIC_ASSERT_POW2(CACHE_8_PAGES);
STATIC_ASSERT_POW2(CACHE_16_PAGES);
STATIC_ASSERT_POW2(CACHE_32_PAGES);
STATIC_ASSERT_POW2(CACHE_64_PAGES);
STATIC_ASSERT_POW2(CACHE_128_PAGES);
STATIC_ASSERT_POW2(CACHE_256_PAGES);
STATIC_ASSERT_POW2(CACHE_512_PAGES);
STATIC_ASSERT_POW2(CACHE_1024_PAGES);


// cache entries
#define CACHE_8_ENTRIES 2014
#define CACHE_16_ENTRIES 1015
#define CACHE_32_ENTRIES 509
#define CACHE_64_ENTRIES 255
#define CACHE_128_ENTRIES 127
#define CACHE_256_ENTRIES 63
#define CACHE_512_ENTRIES 31
#define CACHE_1024_ENTRIES 31


#define ENTRY_SIZE(cache_malloc_size) ((cache_malloc_size) / sizeof(uint64_t))
#define BITFIELD_COUNT(entries) (BITFIELD_COUNT_FOR(entries, bitfield64))
#define BF_BITS BITFIELD_CAPACITY(bitfield64)


void cache_malloc_init();
bool cache_malloc_size_from_ptr(void* ptr, cache_malloc_size* out);
