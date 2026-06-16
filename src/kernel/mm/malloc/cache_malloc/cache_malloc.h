#pragma once

#include <kernel/mm.h>
#include <stdint.h>


#define MIN_CACHE_SIZE CACHE_8
#define MAX_CACHE_SIZE CACHE_2048


#define CACHE_MALLOC_SUPPORTED_SIZE_COUNT 9

// pages per cache
#define STATIC_ASSERT_POW2(N)                                                  \
    _Static_assert(                                                            \
        ((N) & ((N) - 1)) == 0,                                                \
        "cache page count must be of order 2 as page_free relies on aligning " \
        "down to "                                                             \
        "the cache size and needs the va to be aligned to the size")

#define CACHE_8_PAGES    4
#define CACHE_16_PAGES   4
#define CACHE_32_PAGES   4
#define CACHE_64_PAGES   4
#define CACHE_128_PAGES  4
#define CACHE_256_PAGES  4
#define CACHE_512_PAGES  4
#define CACHE_1024_PAGES 8
#define CACHE_2048_PAGES 8

STATIC_ASSERT_POW2(CACHE_8_PAGES);
STATIC_ASSERT_POW2(CACHE_16_PAGES);
STATIC_ASSERT_POW2(CACHE_32_PAGES);
STATIC_ASSERT_POW2(CACHE_64_PAGES);
STATIC_ASSERT_POW2(CACHE_128_PAGES);
STATIC_ASSERT_POW2(CACHE_256_PAGES);
STATIC_ASSERT_POW2(CACHE_512_PAGES);
STATIC_ASSERT_POW2(CACHE_1024_PAGES);
STATIC_ASSERT_POW2(CACHE_2048_PAGES);

// cache entries
#define CACHE_8_ENTRIES    2013
#define CACHE_16_ENTRIES   1014
#define CACHE_32_ENTRIES   509
#define CACHE_64_ENTRIES   255
#define CACHE_128_ENTRIES  127
#define CACHE_256_ENTRIES  63
#define CACHE_512_ENTRIES  31
#define CACHE_1024_ENTRIES 31
#define CACHE_2048_ENTRIES 15


#define ENTRY_SIZE(cache_malloc_size) ((cache_malloc_size) / sizeof(uint64_t))
#define BITFIELD_COUNT(entries)       (BITFIELD_COUNT_FOR(entries, bitfield64))
#define BF_BITS                       BITFIELD_CAPACITY(bitfield64)

constexpr uint64_t CACHE_ID_MASK  = 0xF;
constexpr uint64_t CACHE_ID_MAGIC = 0xAAAAAAAAAAAAAAAA & ~CACHE_ID_MASK;

void cache_malloc_init();
