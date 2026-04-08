#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdmacros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../cache_malloc/cache_malloc.h"


void* kmalloc(size_t bytes)
{
    size_t min_cache_fit = 0;

    for (size_t i = 0; i < CACHE_MALLOC_SUPPORTED_SIZES; i++) {
        size_t cache_size = power_of2(log2_floor((uint32_t)MIN_CACHE) + i);

        if (bytes > cache_size)
            continue;

        min_cache_fit = cache_size;
        break;
    }


    if (min_cache_fit != 0)
        return cache_malloc(min_cache_fit);


    // cannot allocate with the cache allocator, alloc raw pages
    raw_kmalloc_cfg cfg = RAW_KMALLOC_DYNAMIC_CFG;
    cfg.init_zeroed     = true;
    return raw_kmalloc(div_ceil(bytes, PAGE_SIZE), "kmalloc page", &cfg);
}


void kfree(void* ptr)
{
    DEBUG_ASSERT(is_kva_ptr(ptr));

    if (mm_va_is_in_kmap_range(ptr)) {
        // if it is kmapped it must be a cache allocation as it uses kmap
        DEBUG_ASSERT(is_kva_ptr(ptr));

        cache_malloc_size size;
        bool              result = cache_malloc_size_from_ptr(ptr, &size);
        ASSERT(result, "kfree: invalid ptr provided");

        cache_free(size, ptr);
    }
    else {
        // kmalloc with sizes bigger than MAX_CACHE are allocated as dynamic
        raw_kfree(ptr);
    }
}
