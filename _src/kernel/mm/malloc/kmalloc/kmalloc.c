#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/math.h>
#include <stddef.h>

#include "../cache_malloc/cache_malloc.h"

void* kmalloc(size_t bytes)
{
    if (bytes > MAX_CACHE_SIZE) {
        return raw_kmalloc(
            div_ceil(bytes, PAGE_SIZE),
            "kmalloc page",
            RAW_KMALLOC_DYNAMIC_CFG,
            NULL);
    }

    size_t cache_class_size = max(MIN_CACHE_SIZE, next_pow2(bytes));
    return cache_malloc(cache_class_size);
}

void kfree(void* ptr)
{
    DEBUG_ASSERT(is_kva_ptr(ptr));

    if (mm_va_is_in_kmap_range(ptr))
        // if it is kmapped it must be a cache allocation as it uses kmap
        cache_free(ptr);

    else
        // kmalloc with sizes bigger than MAX_CACHE_SIZE are allocated as dynamic
        raw_kfree(ptr);
}

void* kzalloc(size_t bytes)
{
    void* ptr = kmalloc(bytes);
    memzero(ptr, bytes);
    return ptr;
}
