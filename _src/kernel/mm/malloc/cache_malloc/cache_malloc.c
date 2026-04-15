#include "cache_malloc.h"

#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/align.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdattribute.h>
#include <lib/stdbitfield.h>
#include <lib/stdmacros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../malloc/raw_kmalloc/raw_kmalloc.h"
#include "lib/branch.h"
#include "lib/lock.h"

typedef struct cache8 {
    uint64_t       buf[CACHE_8_ENTRIES][ENTRY_SIZE(CACHE_8)];
    bitfield64     reserved[BITFIELD_COUNT(CACHE_8_ENTRIES)];
    struct cache8* prev;
    struct cache8* next;
} cache8;
_Static_assert(sizeof(cache8) <= CACHE_8_PAGES * PAGE_SIZE);


typedef struct cache16 {
    uint64_t        buf[CACHE_16_ENTRIES][ENTRY_SIZE(CACHE_16)];
    bitfield64      reserved[BITFIELD_COUNT(CACHE_16_ENTRIES)];
    struct cache16* prev;
    struct cache16* next;
} cache16;
_Static_assert(sizeof(cache16) <= CACHE_16_PAGES * PAGE_SIZE);


typedef struct cache32 {
    uint64_t        buf[CACHE_32_ENTRIES][ENTRY_SIZE(CACHE_32)];
    bitfield64      reserved[BITFIELD_COUNT(CACHE_32_ENTRIES)];
    struct cache32* prev;
    struct cache32* next;
} cache32;
_Static_assert(sizeof(cache32) <= CACHE_32_PAGES * PAGE_SIZE);


typedef struct cache64 {
    uint64_t        buf[CACHE_64_ENTRIES][ENTRY_SIZE(CACHE_64)];
    bitfield64      reserved[BITFIELD_COUNT(CACHE_64_ENTRIES)];
    struct cache64* prev;
    struct cache64* next;
} cache64;
_Static_assert(sizeof(cache64) <= CACHE_64_PAGES * PAGE_SIZE);


typedef struct cache128 {
    uint64_t         buf[CACHE_128_ENTRIES][ENTRY_SIZE(CACHE_128)];
    bitfield64       reserved[BITFIELD_COUNT(CACHE_128_ENTRIES)];
    struct cache128* prev;
    struct cache128* next;
} cache128;
_Static_assert(sizeof(cache128) <= CACHE_128_PAGES * PAGE_SIZE);


typedef struct cache256 {
    uint64_t         buf[CACHE_256_ENTRIES][ENTRY_SIZE(CACHE_256)];
    bitfield64       reserved[BITFIELD_COUNT(CACHE_256_ENTRIES)];
    struct cache256* prev;
    struct cache256* next;
} cache256;
_Static_assert(sizeof(cache256) <= CACHE_256_PAGES * PAGE_SIZE);


typedef struct cache512 {
    uint64_t         buf[CACHE_512_ENTRIES][ENTRY_SIZE(CACHE_512)];
    bitfield64       reserved[BITFIELD_COUNT(CACHE_512_ENTRIES)];
    struct cache512* prev;
    struct cache512* next;
} cache512;
_Static_assert(sizeof(cache512) <= CACHE_512_PAGES * PAGE_SIZE);


typedef struct cache1024 {
    uint64_t          buf[CACHE_1024_ENTRIES][ENTRY_SIZE(CACHE_1024)];
    bitfield64        reserved[BITFIELD_COUNT(CACHE_1024_ENTRIES)];
    struct cache1024* prev;
    struct cache1024* next;
} cache1024;
_Static_assert(sizeof(cache1024) <= CACHE_1024_PAGES * PAGE_SIZE);


typedef struct {
    uint64_t*   buf;
    bitfield64* reserved;
    uintptr_t*  prev;
    uintptr_t*  next;
} cache_fields;


typedef struct cache_descriptor {
    size_t entry_size;
    size_t entries;
    size_t bitfields;
} cache_descriptor;


typedef struct {
    cache_malloc_size size;
    void*             first_cache;
    void*             last_cache;
    struct {
        void*  cache;
        size_t bf_n;
        size_t bf_i;
    } first_free_cache;
    size_t cache_count;
} cache_malloc_state;


static const char* CACHE_ALLOCATION_TAGS[CACHE_MALLOC_SUPPORTED_SIZES] = {
    "cache malloc 8",
    "cache malloc 16",
    "cache malloc 32",
    "cache malloc 64",
    "cache malloc 128",
    "cache malloc 256",
    "cache malloc 512",
    "cache malloc 1024",
};

// this array must stay ordered from smaller to bigger
static constexpr size_t CACHE_PAGE_SIZES[2] = {4, 8};

static constexpr size_t CACHE_PAGES[CACHE_MALLOC_SUPPORTED_SIZES] = {
    CACHE_8_PAGES,
    CACHE_16_PAGES,
    CACHE_32_PAGES,
    CACHE_64_PAGES,
    CACHE_128_PAGES,
    CACHE_256_PAGES,
    CACHE_512_PAGES,
    CACHE_1024_PAGES,
};


static constexpr size_t CACHE_ENTRIES[CACHE_MALLOC_SUPPORTED_SIZES] = {
    CACHE_8_ENTRIES,
    CACHE_16_ENTRIES,
    CACHE_32_ENTRIES,
    CACHE_64_ENTRIES,
    CACHE_128_ENTRIES,
    CACHE_256_ENTRIES,
    CACHE_512_ENTRIES,
    CACHE_1024_ENTRIES,
};


static constexpr size_t CACHE_BITFIELDS[CACHE_MALLOC_SUPPORTED_SIZES] = {
    BITFIELD_COUNT(CACHE_8_ENTRIES),
    BITFIELD_COUNT(CACHE_16_ENTRIES),
    BITFIELD_COUNT(CACHE_32_ENTRIES),
    BITFIELD_COUNT(CACHE_64_ENTRIES),
    BITFIELD_COUNT(CACHE_128_ENTRIES),
    BITFIELD_COUNT(CACHE_256_ENTRIES),
    BITFIELD_COUNT(CACHE_512_ENTRIES),
    BITFIELD_COUNT(CACHE_1024_ENTRIES),
};

static spinlock_t lock;

static inline cache_fields
get_generic_fields(cache_malloc_size size, void* cache_ptr)
{
    typedef union {
        cache8    c8;
        cache16   c16;
        cache32   c32;
        cache64   c64;
        cache128  c128;
        cache256  c256;
        cache512  c512;
        cache1024 c1024;
    } cache_union;

    cache_union* u = (cache_union*)cache_ptr;
    cache_fields c;

#define DECLARE_CACHE(size)                      \
    c.buf      = &u->c##size.buf[0][0];          \
    c.reserved = &u->c##size.reserved[0];        \
    c.prev     = (uintptr_t*)(&u->c##size.prev); \
    c.next     = (uintptr_t*)(&u->c##size.next); \
    break

    switch (size) {
        case CACHE_8:
            DECLARE_CACHE(8);
        case CACHE_16:
            DECLARE_CACHE(16);
        case CACHE_32:
            DECLARE_CACHE(32);
        case CACHE_64:
            DECLARE_CACHE(64);
        case CACHE_128:
            DECLARE_CACHE(128);
        case CACHE_256:
            DECLARE_CACHE(256);
        case CACHE_512:
            DECLARE_CACHE(512);
        case CACHE_1024:
            DECLARE_CACHE(1024);
    }

    return c;

#undef DECLARE_CACHE
}


static constexpr raw_kmalloc_cfg CACHE_MALLOC_RAW_KMALLOC_CFG = {
    .assign_pa    = true,
    .fill_reserve = true,
    .device_mem   = false,
    .permanent    = false,
    .kmap = true, // needs to be kmapped for allowing kmalloc to easily know the
                  // pa and ask the page allocator for data
    .init_zeroed = true,
};


static cache_malloc_state state[CACHE_MALLOC_SUPPORTED_SIZES];


void cache_malloc_init()
{
    size_t log = log2_floor((uint32_t)CACHE_8);

    for (size_t i = 0; i < CACHE_MALLOC_SUPPORTED_SIZES; i++) {
        state[i] = (cache_malloc_state) {
            .size             = power_of2(log),
            .first_cache      = NULL,
            .last_cache       = NULL,
            .first_free_cache = {.cache = NULL, .bf_n = 0, .bf_i = 0},
            .cache_count      = 0,
        };

        log++;
    }
}


static inline size_t cache_idx_from_size(cache_malloc_size size)
{
    ASSERT(size >= CACHE_8 && size <= CACHE_1024);
    return log2_floor(size) - log2_floor((uint32_t)MIN_CACHE);
}


static inline void* new_cache(cache_malloc_size size, void* prev)
{
    size_t i   = cache_idx_from_size(size);
    void*  ptr = raw_kmalloc(
        CACHE_PAGES[i],
        CACHE_ALLOCATION_TAGS[i],
        &CACHE_MALLOC_RAW_KMALLOC_CFG);

    DEBUG_ASSERT(((uintptr_t)ptr % (CACHE_PAGES[i] * PAGE_SIZE)) == 0);

    cache_fields c = get_generic_fields(size, ptr);

    *c.prev = (uintptr_t)prev;
    *c.next = 0;

    state[i].cache_count++;


    // set the page allocator data
    mm_page_data data;
    puintptr_t   pa = (puintptr_t)kva_to_kpa_pt(ptr);


    raw_kmalloc_lock();


    dbgT(bool) result = page_allocator_get_data(pa, &data);
    DEBUG_ASSERT(result);

    data.cache_size = log2_floor_u32(size);

    result = page_allocator_set_data(pa, data);
    DEBUG_ASSERT(result);


    raw_kmalloc_unlock(NULL);

    return ptr;
}


// both bf and n must be initialized with the wanted start idx
static inline bool find_empty_slot(
    cache_malloc_size size,
    cache_fields      c,
    size_t*           bf_n,
    size_t*           bf_i)
{
    size_t      i          = cache_idx_from_size(size);
    bitfield64* r          = c.reserved;
    bool        first_iter = true;

    for (; *bf_n < CACHE_BITFIELDS[i]; (*bf_n)++) {
        bool full = (*bf_n < (CACHE_ENTRIES[i] / BF_BITS));

        // the last bitfield might not have all its
        // bits as valid or useful bits, they must allways be zero
        size_t     entries = full ? BF_BITS : CACHE_ENTRIES[i] % BF_BITS;
        bitfield64 full_mask =
            entries >= 64 ? ~(bitfield64)0 : ((1ULL << entries)) - 1;

        if (c.reserved[*bf_n] == full_mask) {
            first_iter = false;
            continue;
        }

        for (*bf_i = first_iter ? *bf_i : 0; *bf_i < entries; (*bf_i)++)
            if (!bitfield_get(r[*bf_n], *bf_i))
                return true;

        first_iter = false;
    }

    return false;
}


void* cache_malloc(cache_malloc_size size)
{
    size_t              i = cache_idx_from_size(size);
    cache_malloc_state* s = &state[i];

    size_t bf_n, bf_i;

    void* cache_ptr;
    void* result;

    irq_spinlocked(&lock)
    {
        if (s->first_free_cache.cache) {
            cache_ptr = s->first_free_cache.cache;
            bf_n      = s->first_free_cache.bf_n;
            bf_i      = s->first_free_cache.bf_i;
        }
        else {
            cache_ptr = new_cache(size, s->last_cache);

            if (s->first_cache == NULL) {
                DEBUG_ASSERT(!s->last_cache);
                s->first_cache = cache_ptr;
            }
            else {
                cache_fields last_f = get_generic_fields(size, s->last_cache);
                *last_f.next        = (uintptr_t)cache_ptr;
            }

            s->last_cache             = cache_ptr;
            s->first_free_cache.cache = cache_ptr;

            bf_n = 0;
            bf_i = 0;
        }

        cache_fields c = get_generic_fields(size, cache_ptr);

        bool slot_found = find_empty_slot(size, c, &bf_n, &bf_i);
        ASSERT(slot_found);
        bitfield_set_high(c.reserved[bf_n], bf_i);

        size_t entry = bf_n * BF_BITS + bf_i;
        result       = &c.buf[entry * ENTRY_SIZE(size)];
        DEBUG_ASSERT((puintptr_t)result % size == 0);

        DEBUG_ASSERT(s->first_cache);

        // check if the cache still has empty slots, start from the last
        // iterated indexes
        slot_found = find_empty_slot(size, c, &bf_n, &bf_i);

        // still has empty slots
        if (slot_found) {
            s->first_free_cache.cache = cache_ptr;
            s->first_free_cache.bf_n  = bf_n;
            s->first_free_cache.bf_i  = bf_i;

            return result;
        }


        // the current container is full, so search for the first container with
        // an empty spot
        void* cur = s->first_cache;
        while (cur) {
            c = get_generic_fields(size, cur);

            bf_n = 0;
            bf_i = 0;

            slot_found = find_empty_slot(size, c, &bf_n, &bf_i);

            if (slot_found) {
                s->first_free_cache.cache = cur;
                s->first_free_cache.bf_n  = bf_n;
                s->first_free_cache.bf_i  = bf_i;

                return result;
            }

            cur = (void*)(*c.next);
        }

        // there is no cache with an available slot, either a new free is made
        // or the next reserved_slot call will allocate another cache
        s->first_free_cache.cache = NULL;
        s->first_free_cache.bf_n  = 0;
        s->first_free_cache.bf_i  = 0;

        return result;
    }

#ifdef DEBUG
    PANIC("should be unreachable");
#else
    unreachable();
#endif
}


void cache_free(cache_malloc_size size, void* ptr)
{
    size_t              i = cache_idx_from_size(size);
    cache_malloc_state* s = &state[i];

    void* const cache_ptr = align_down_pt(ptr, CACHE_PAGES[i] * PAGE_SIZE);

    irq_spinlocked(&lock)
    {
        cache_fields f = get_generic_fields(size, cache_ptr);

        DEBUG_ASSERT(((uintptr_t)ptr - (uintptr_t)&f.buf[0]) % size == 0);
        size_t entry_idx = ((uintptr_t)ptr - (uintptr_t)&f.buf[0]) / size;

        DEBUG_ASSERT(entry_idx < CACHE_ENTRIES[i]);

        size_t bf_i = entry_idx / BF_BITS;
        size_t bf_n = entry_idx % BF_BITS;

        ASSERT(bitfield_get(f.reserved[bf_i], bf_n), "cache_free: double free");
        bitfield_clear(f.reserved[bf_i], bf_n);

        bool empty = true;
        for (size_t j = 0; j < CACHE_BITFIELDS[i]; j++) {
            if (f.reserved[j] != 0) {
                empty = false;
                break;
            }
        }

        if (likely(!empty)) {
            if (!s->first_free_cache.cache) {
                s->first_free_cache.cache = cache_ptr;
                s->first_free_cache.bf_i  = bf_i;
                s->first_free_cache.bf_n  = bf_n;
            }
            return;
        }


        if (*f.prev) {
            cache_fields prev_f = get_generic_fields(size, (void*)(*f.prev));
            *prev_f.next        = *f.next;
        }
        else {
            s->first_cache = (void*)(*f.next);
        }

        if (*f.next) {
            cache_fields next_f = get_generic_fields(size, (void*)(*f.next));
            *next_f.prev        = *f.prev;
        }
        else {
            s->last_cache = (void*)(*f.prev);
        }

        if (s->first_free_cache.cache == cache_ptr) {
            s->first_free_cache.cache = NULL;
            s->first_free_cache.bf_i  = 0;
            s->first_free_cache.bf_n  = 0;
        }

        s->cache_count--;

        DEBUG_ASSERT(
            (s->cache_count == 0 && s->first_cache == NULL &&
             s->last_cache == NULL) ||
            (s->cache_count > 0 && s->first_cache != NULL &&
             s->last_cache != NULL));

        if (!s->first_free_cache.cache) {
            void* cur = s->first_cache;
            while (cur) {
                cache_fields cur_f = get_generic_fields(size, cur);

                size_t n = 0, k = 0;
                if (find_empty_slot(size, cur_f, &n, &k)) {
                    s->first_free_cache.cache = cur;
                    s->first_free_cache.bf_n  = n;
                    s->first_free_cache.bf_i  = k;
                    break;
                }

                cur = (void*)(*cur_f.next);
            }
        }
    }

    raw_kfree(cache_ptr);
}




bool cache_malloc_size_from_ptr(void* ptr, cache_malloc_size* out)
{
    for (size_t i = 0; i < ARRAY_LEN(CACHE_PAGE_SIZES); i++) {
        void* base = align_down_pt(ptr, PAGE_SIZE * CACHE_PAGE_SIZES[i]);

        mm_page_data data;
        bool         result = page_allocator_get_data((puintptr_t)base, &data);

        if (!result)
            continue;

        size_t log = data.cache_size;

        if (log == 0)
            continue;

        DEBUG_ASSERT(
            log >= log2_floor_u32(MIN_CACHE) &&
            log <= log2_floor_u32(MAX_CACHE));

        *out = power_of2(log);

        return true;
    }

    return false;
}
