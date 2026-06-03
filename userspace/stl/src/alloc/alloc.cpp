extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
}

#pragma GCC visibility push(internal)

class Lock {
    int flag;

  public:

    Lock() : flag(0) {}

    __attribute__((always_inline)) inline void lock_acquire()
    {
        __asm__ volatile("1:                             \n"

                         "2:                             \n"
                         "    ldar    w1, %[flag]        \n"
                         "    cbnz    w1, 3f             \n"
                         "    b       4f                 \n"

                         "3:                             \n"
                         "    bl      syscall_yield      \n"
                         "    b       2b                 \n"

                         "4:                             \n"
                         "    ldaxr   w1, %[flag]        \n"
                         "    cbnz    w1, 2b             \n"

                         "    mov     w2, #1             \n"
                         "    stlxr   w3, w2, %[flag]   \n"
                         "    cbnz    w3, 1b             \n"
                         :
                         : [flag] "Q"(this->flag)
                         : "w1", "w2", "w3", "lr", "memory");
    }

    __attribute__((always_inline)) inline void lock_release()
    {
        __asm__ volatile("    mov     w1, #0             \n"
                         "    stlr    w1, %[flag]        \n"
                         :
                         : [flag] "Q"(this->flag)
                         : "w1", "memory");
    }
};

enum MmallocType {
    MALLOC_CACHE,
    MALLOC_MMAP,
};

struct CacheAllocation {
    void* arena;
    void* pt;
};

struct MmapAllocation {
    size_t length;
    void* pt;
};

union StlAllocationData {
    struct CacheAllocation cache_data;
    struct MmapAllocation mmap_data;

    struct {
        size_t _;
        void* pt;
    } _;
};

struct StlAllocation {
    enum MmallocType type;
    uint32_t layout_align;
    size_t layout_size;
    union StlAllocationData data;
};

struct MallocNode { // TODO: Use a map
    struct MallocNode* next;
    CacheAllocation node_allocation; // the token for the node itself
    StlAllocation allocation;
};

typedef uint8_t byte_t;
static constexpr size_t PAGE_SIZE = 4096;

namespace panic {
[[gnu::cold, __noreturn__]] static void stl_panic(const char* panic_msg)
{
    print(panic_msg);
    exit(1);
}
} // namespace panic

#define _STRINGIFY(x) #x
#define _TOSTRING(x) _STRINGIFY(x)

#define stl_panic()                                  \
    panic::stl_panic(                                \
        "[stl] allocation panic!\n\rfile: " __FILE__ \
        "\n\rline: " _TOSTRING(__LINE__) "\n\r")

template <size_t ENTRIES>
class CacheEntriesBitmap {
  public:

    static constexpr size_t WORD_BITS = 64;
    static constexpr size_t BM_WORDS = (ENTRIES + WORD_BITS - 1) / WORD_BITS;

  private:

    static constexpr size_t LAST_BM_ENTRIES =
        (ENTRIES % WORD_BITS == 0) ? WORD_BITS : (ENTRIES % WORD_BITS);

    static constexpr uint64_t MASK = UINT64_MAX;
    static constexpr uint64_t LAST_MASK =
        (LAST_BM_ENTRIES == 64) ? UINT64_MAX
                                : (UINT64_C(1) << LAST_BM_ENTRIES) - 1;

    uint64_t bitmap[BM_WORDS];

  public:

    static constexpr size_t bm_words() { return BM_WORDS; }

    int64_t try_push()
    {
        size_t i, j;

        for (i = 0; i < BM_WORDS; i++) {
            uint64_t mask = (i != BM_WORDS - 1) ? MASK : LAST_MASK;
            uint64_t word = this->bitmap[i];

            if ((word & mask) == mask) {
                // word is full
                continue;
            }

            j = __builtin_ctzll(~word & mask);

            this->bitmap[i] |= UINT64_C(1) << j;

            return static_cast<int64_t>(i * WORD_BITS + j);
        }

        return -1;
    }

    bool pop(size_t idx)
    {
        if (idx >= ENTRIES)
            return false;

        size_t i = idx / WORD_BITS;
        size_t j = idx % WORD_BITS;

        if (((this->bitmap[i] >> j) & 1) == 0) // it was already free
            return false;

        this->bitmap[i] &= ~(UINT64_C(1) << j);

        return true;
    }

    bool is_full()
    {
        for (size_t i = 0; i < BM_WORDS; i++) {
            uint64_t mask = (i != BM_WORDS - 1) ? MASK : LAST_MASK;
            uint64_t word = this->bitmap[i];

            if (word != mask)
                return false;
        }

        return true;
    };

    bool is_empty()
    {
        for (size_t i = 0; i < BM_WORDS; i++) {
            uint64_t word = this->bitmap[i];

            if (word != 0)
                return false;
        }

        return true;
    }

    size_t count_used()
    {
        size_t n = 0;
        for (size_t i = 0; i < BM_WORDS; i++)
            n += __builtin_popcountll(this->bitmap[i]);

        return n;
    }
};

struct ArenaCfg {
    size_t N;
    size_t PAGES;
};

template <ArenaCfg Cfg>
class CacheArena {
    static constexpr size_t N = Cfg.N;
    static constexpr size_t PAGES = Cfg.PAGES;

    static_assert(PAGES != 0, "Pages cannot be zero");
    static_assert(N != 0, "N cannot be zero");
    static_assert((N & (N - 1)) == 0, "N must be power of 2");

    static constexpr size_t TOTAL_BYTES = PAGES * PAGE_SIZE;
    static constexpr size_t _header_sz = sizeof(size_t) + 2 * sizeof(void*);

    static constexpr size_t _approx_entries = (TOTAL_BYTES - _header_sz) / N;
    static constexpr size_t _bitmap_words = (_approx_entries + 63) / 64;
    static constexpr size_t _bitmap_bytes = _bitmap_words * sizeof(uint64_t);

    static constexpr size_t ENTRIES =
        (TOTAL_BYTES - _header_sz - _bitmap_bytes) / N;
    static constexpr size_t PADDING =
        TOTAL_BYTES - _header_sz - _bitmap_bytes - ENTRIES * N;

    static constexpr size_t sizeof_cache() { return sizeof(CacheArena<Cfg>); }

    inline static Lock lock = Lock();
    inline static CacheArena<Cfg>* tail = nullptr;
    inline static CacheArena<Cfg>* best_fit = nullptr;
    inline static CacheArena<Cfg>* head = nullptr;

    size_t slot_size; // must be the first item of the class to identify it
    CacheArena<Cfg>*prev, *next;
    CacheEntriesBitmap<ENTRIES> reserved;
    [[maybe_unused]] byte_t _[PADDING];
    alignas(N) byte_t data[ENTRIES][N];

    static CacheArena<Cfg>* new_tail()
    {
        int64_t mmap = syscall_mmap(
            NULL,
            PAGES * PAGE_SIZE,
            MMAP_PROT_READ | MMAP_PROT_WRITE,
            MMAP_FLAG_ANONYMOUS,
            0,
            0); // comes memzeroed

        if (mmap <= 0)
            stl_panic();

        CacheArena<Cfg>* new_arena =
            reinterpret_cast<CacheArena<Cfg>*>(static_cast<uintptr_t>(mmap));

        if (head == nullptr) {
            head = new_arena;
        }

        if (tail == nullptr) {
            tail = new_arena;
        }
        else {
            tail->next = new_arena;
            new_arena->prev = tail;

            tail = new_arena;
        }

        if (best_fit == nullptr)
            best_fit = new_arena;

        new_arena->slot_size = N;

        return tail;
    }

    static void move_to_head(CacheArena<Cfg>* arena)
    {
        if (arena == head)
            return;

        if (arena->prev)
            arena->prev->next = arena->next;

        if (arena->next)
            arena->next->prev = arena->prev;

        if (arena == tail)
            tail = arena->prev;

        arena->prev = nullptr;
        arena->next = head;

        if (head)
            head->prev = arena;

        head = arena;
    }

    static void move_to_tail(CacheArena<Cfg>* arena)
    {
        if (arena == tail)
            return;

        if (arena->prev)
            arena->prev->next = arena->next;

        if (arena->next)
            arena->next->prev = arena->prev;

        if (arena == head)
            head = arena->next;

        arena->next = nullptr;
        arena->prev = tail;

        if (tail)
            tail->next = arena;

        tail = arena;
    }

    static void update_best_fit(CacheArena<Cfg>* from)
    {
        CacheArena<Cfg>* arena = from;

        if (!from) {
            stl_panic();
        }

        // forward search
        while (arena != nullptr && arena->reserved.is_full()) {
            arena = arena->next;
        }

        if (arena != nullptr) {
            best_fit = arena;
            return;
        }

        // backwards search
        arena = from->prev;

        while (arena != nullptr && arena->reserved.is_full()) {
            arena = arena->prev;
        }

        if (arena != nullptr) {
            best_fit = arena;
            return;
        }

        best_fit = nullptr;
    }

  public:

    static constexpr size_t OFFSETOF_DATA =
        __builtin_offsetof(CacheArena<Cfg>, data);

    static constexpr bool valid_size()
    {
        return sizeof_cache() == PAGE_SIZE * PAGES;
    }
    static constexpr bool valid_words()
    {
        return _bitmap_words == CacheEntriesBitmap<ENTRIES>::bm_words();
    }
    static constexpr bool is_valid()
    {
        return valid_size() && ENTRIES > 0 && _bitmap_bytes >= 8 &&
               valid_words();
    }

    static constexpr size_t entries() { return ENTRIES; }
    static constexpr size_t padding() { return PADDING; }
    static constexpr size_t bm_words() { return _bitmap_words; }
    static constexpr double data_percentage()
    {
        return ((double)(ENTRIES * N) / (double)TOTAL_BYTES) * 100.0;
    }

    static CacheAllocation push()
    {
        if (best_fit == nullptr)
            best_fit = new_tail();

        CacheArena<Cfg>* available = best_fit;
        int64_t idx = available->reserved.try_push();

        if (idx < 0 || static_cast<size_t>(idx) >= ENTRIES)
            stl_panic();

        if (available->reserved.is_full()) {
            move_to_head(available);
            update_best_fit(tail);
        }

        return CacheAllocation(available, &available->data[idx]);
    }

    static bool pop(void* arena, void* pt)
    {
        size_t idx = (static_cast<byte_t*>(pt) -
                      (static_cast<byte_t*>(arena) + OFFSETOF_DATA)) /
                     N;

        CacheArena<Cfg>* a = static_cast<CacheArena<Cfg>*>(arena);

        bool was_full = a->reserved.is_full();
        bool freed = a->reserved.pop(idx);

        if (!freed)
            return false; // double free

        if (a->reserved.is_empty()) {
            CacheArena* prev_arena = a->prev;
            CacheArena* next_arena = a->next;

            if (prev_arena)
                prev_arena->next = next_arena;

            if (next_arena)
                next_arena->prev = prev_arena;

            if (a == head)
                head = next_arena;

            if (a == tail)
                tail = prev_arena;

            if (best_fit == a) {
                if (next_arena)
                    update_best_fit(next_arena);
                else if (prev_arena)
                    update_best_fit(prev_arena);
                else
                    best_fit = nullptr;
            }

            if (syscall_munmap(a, PAGES * PAGE_SIZE) < 0)
                stl_panic();

            return true;
        }

        if (was_full) {
            move_to_tail(a);

            if (best_fit == nullptr ||
                a->reserved.count_used() < best_fit->reserved.count_used()) {
                best_fit = a;
            }
        }

        return true;
    }

#ifdef TESTING

    static size_t count_arenas()
    {
        size_t count = 0;
        auto* arena = CacheArena<Cfg>::head;
        while (arena != nullptr) {
            count++;
            arena = arena->next;
        }
        return count;
    }

    static size_t count_used_entries()
    {
        size_t count = 0;
        auto* arena = CacheArena<Cfg>::head;
        while (arena != nullptr) {
            count += arena->reserved.count_used();
            arena = arena->next;
        }
        return count;
    }

#endif
};

static constexpr ArenaCfg CACHE4_CFG = ArenaCfg(4, 1);
static constexpr ArenaCfg CACHE8_CFG = ArenaCfg(8, 1);
static constexpr ArenaCfg CACHE16_CFG = ArenaCfg(16, 1);
static constexpr ArenaCfg CACHE32_CFG = ArenaCfg(32, 1);
static constexpr ArenaCfg CACHE64_CFG = ArenaCfg(64, 2);
static constexpr ArenaCfg CACHE128_CFG = ArenaCfg(128, 2);
static constexpr ArenaCfg CACHE256_CFG = ArenaCfg(256, 3);
static constexpr ArenaCfg CACHE512_CFG = ArenaCfg(512, 3);
static constexpr ArenaCfg CACHE1024_CFG = ArenaCfg(1024, 5);
static constexpr ArenaCfg CACHE2048_CFG = ArenaCfg(2048, 10);

static_assert(CacheArena<CACHE4_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE4_CFG>::entries() > 0);
static_assert(CacheArena<CACHE4_CFG>::data_percentage() >= 95.0);

static_assert(CacheArena<CACHE8_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE8_CFG>::entries() > 0);
static_assert(CacheArena<CACHE8_CFG>::data_percentage() >= 95.0);

static_assert(CacheArena<CACHE16_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE16_CFG>::entries() > 0);
static_assert(CacheArena<CACHE16_CFG>::data_percentage() >= 95.0);

static_assert(CacheArena<CACHE32_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE32_CFG>::entries() > 0);
static_assert(CacheArena<CACHE32_CFG>::data_percentage() >= 95.0);

static_assert(CacheArena<CACHE64_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE64_CFG>::entries() > 0);
static_assert(CacheArena<CACHE64_CFG>::data_percentage() >= 95.0);

static_assert(CacheArena<CACHE128_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE128_CFG>::entries() > 0);
static_assert(CacheArena<CACHE128_CFG>::data_percentage() >= 95.0);

static_assert(CacheArena<CACHE256_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE256_CFG>::entries() > 0);
static_assert(CacheArena<CACHE256_CFG>::data_percentage() >= 95.0);

static_assert(CacheArena<CACHE512_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE512_CFG>::entries() > 0);
static_assert(CacheArena<CACHE512_CFG>::data_percentage() >= 95.0);

static_assert(CacheArena<CACHE1024_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE1024_CFG>::entries() > 0);
static_assert(CacheArena<CACHE1024_CFG>::data_percentage() >= 95.0);

static_assert(CacheArena<CACHE2048_CFG>::is_valid(), "!is_valid_size");
static_assert(CacheArena<CACHE2048_CFG>::entries() > 0);
static_assert(CacheArena<CACHE2048_CFG>::data_percentage() >= 95.0);

static constexpr size_t next_pow2_exp(size_t bytes)
{
    return 64 - __builtin_clzll(bytes - 1);
}

static bool stl_cache_free(CacheAllocation info);
static CacheAllocation stl_cache_malloc(size_t bytes);

static struct MallocNode* head = NULL;

static void stl_push_allocation_data(StlAllocation allocation)
{
    CacheAllocation cache = stl_cache_malloc(sizeof(struct MallocNode));

    struct MallocNode* node = (struct MallocNode*)cache.pt;

    *node = (struct MallocNode) {
        .next = head,
        .node_allocation = cache,
        .allocation = allocation,
    };

    head = node;
}

static bool stl_pop_allocation_data(
    void* addr,
    size_t layout_size,
    size_t layout_align,
    StlAllocation* allocation)
{
    struct MallocNode* curr = head;
    struct MallocNode* prev = NULL;

    while (curr != NULL && addr != curr->allocation.data._.pt) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr)
        return false;

    if (curr == head) {
        head = curr->next;
    }
    else {
        prev->next = curr->next;
    }

    if (layout_size || layout_align) // if both zero unchecked
        if (curr->allocation.layout_size != layout_size ||
            curr->allocation.layout_align != layout_align) {
            stl_panic();
        }

    *allocation = curr->allocation;

    return stl_cache_free(curr->node_allocation);
}

static CacheAllocation stl_cache_malloc(size_t bytes)
{
    if (bytes > 2048)
        return CacheAllocation(NULL, NULL);

    size_t order = next_pow2_exp(bytes >= 4 ? bytes : 4);

    switch (order) {
        case 2:
            return CacheArena<CACHE4_CFG>::push();
        case 3:
            return CacheArena<CACHE8_CFG>::push();
        case 4:
            return CacheArena<CACHE16_CFG>::push();
        case 5:
            return CacheArena<CACHE32_CFG>::push();
        case 6:
            return CacheArena<CACHE64_CFG>::push();
        case 7:
            return CacheArena<CACHE128_CFG>::push();
        case 8:
            return CacheArena<CACHE256_CFG>::push();
        case 9:
            return CacheArena<CACHE512_CFG>::push();
        case 10:
            return CacheArena<CACHE1024_CFG>::push();
        case 11:
            return CacheArena<CACHE2048_CFG>::push();
    }

    stl_panic();
};

static void print_u64(uint64_t value)
{
    static char buf[21]; // max uint64 = 18446744073709551615 (20 digits) + null

    if (value == 0) {
        print("0");
        return;
    }

    int i = 20;
    buf[i] = '\0';

    while (value > 0) {
        buf[--i] = '0' + (value % 10);
        value /= 10;
    }

    print(&buf[i]);
}

static void print_hex(uint64_t value)
{
    static char buf[19]; // "0x" + 16 hex digits + null
    static constexpr char digits[] = "0123456789abcdef";

    int i = 18;
    buf[i] = '\0';

    if (value == 0) {
        print("0x0");
        return;
    }

    while (value > 0) {
        buf[--i] = digits[value & 0xF];
        value >>= 4;
    }

    buf[--i] = 'x';
    buf[--i] = '0';

    print(&buf[i]);
}

static bool stl_cache_free(CacheAllocation info)
{
    void* arena = info.arena;
    void* pt = info.pt;

    switch (*static_cast<size_t*>(arena)) {
        case 4: {
            return CacheArena<CACHE4_CFG>::pop(arena, pt);
        }
        case 8: {
            return CacheArena<CACHE8_CFG>::pop(arena, pt);
        }
        case 16: {
            return CacheArena<CACHE16_CFG>::pop(arena, pt);
        }
        case 32: {
            return CacheArena<CACHE32_CFG>::pop(arena, pt);
        }
        case 64: {
            return CacheArena<CACHE64_CFG>::pop(arena, pt);
        }
        case 128: {
            return CacheArena<CACHE128_CFG>::pop(arena, pt);
        }
        case 256: {
            return CacheArena<CACHE256_CFG>::pop(arena, pt);
        }
        case 512: {
            return CacheArena<CACHE512_CFG>::pop(arena, pt);
        }
        case 1024: {
            return CacheArena<CACHE1024_CFG>::pop(arena, pt);
        }
        case 2048: {
            return CacheArena<CACHE2048_CFG>::pop(arena, pt);
        }
        default:
            stl_panic();
    }
}

static constexpr MmapAllocation MMAP_ALLOCATOR_ERROR = {
    .length = 0,
    .pt = nullptr,
};

static MmapAllocation stl_mmap_malloc(size_t pages)
{
    if (pages == 0)
        return MMAP_ALLOCATOR_ERROR;

    int64_t mmap = syscall_mmap(
        NULL,
        pages * PAGE_SIZE,
        MMAP_PROT_READ | MMAP_PROT_WRITE,
        MMAP_FLAG_ANONYMOUS,
        0,
        0);

    return mmap > 0
               ? (MmapAllocation) {.length = pages * PAGE_SIZE, .pt = (void*)mmap}
               : MMAP_ALLOCATOR_ERROR;
}

static bool stl_mmap_free(MmapAllocation allocation)
{
    int64_t munmap = syscall_munmap(allocation.pt, allocation.length);

    return munmap >= 0;
}

#pragma GCC visibility pop
extern "C" { // API

void* __stl_malloc(size_t layout_size, size_t layout_align, bool zeroed)
{
    StlAllocation allocation;

    if (layout_size == 0)
        return NULL;

    allocation.layout_size = layout_size;
    allocation.layout_align = layout_align;

    size_t size = layout_size >= layout_align ? layout_size : layout_align;

    if (size <= CACHE2048_CFG.N) {
        allocation.type = MALLOC_CACHE;
        allocation.data.cache_data = stl_cache_malloc(size);

        if (zeroed)
            __builtin_memset(allocation.data.cache_data.pt, 0, layout_size);
    }
    else {
        size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

        allocation.type = MALLOC_MMAP;
        allocation.data.mmap_data = stl_mmap_malloc(pages);

        // already comes zeroed
    }

    if (allocation.data._.pt == nullptr)
        return NULL;

    stl_push_allocation_data(allocation);

    return allocation.data._.pt;
}

bool __stl_free(void* addr, size_t layout_size, size_t layout_align)
{
    StlAllocation allocation;
    bool found =
        stl_pop_allocation_data(addr, layout_size, layout_align, &allocation);

    if (!found)
        return false;

    switch (allocation.type) {
        case MALLOC_CACHE: {
            return stl_cache_free(allocation.data.cache_data);
        }
        case MALLOC_MMAP: {
            return stl_mmap_free(allocation.data.mmap_data);
        }
    }

    return false;
}
}

#ifdef TESTING

// --- testing ---
extern "C" {
size_t get_allocation_node_count()
{
    size_t count = 0;
    struct MallocNode* curr = head;
    while (curr != nullptr) {
        count++;
        curr = curr->next;
    }
    return count;
}

size_t get_cache_allocation_count()
{
    size_t count = 0;
    struct MallocNode* curr = head;
    while (curr != nullptr) {
        if (curr->allocation.type == MALLOC_CACHE)
            count++;
        curr = curr->next;
    }
    return count;
}

size_t get_mmap_allocation_count()
{
    size_t count = 0;
    struct MallocNode* curr = head;
    while (curr != nullptr) {
        if (curr->allocation.type == MALLOC_MMAP)
            count++;
        curr = curr->next;
    }
    return count;
}

struct CacheStats {
    size_t arena_count;
    size_t used_entries;
    size_t slot_size;
};

// Stats por cada tamaño de cache
void get_cache_stats(struct CacheStats* out, size_t* out_count)
{
    *out_count = 10;

    out[0] = {
        CacheArena<CACHE4_CFG>::count_arenas(),
        CacheArena<CACHE4_CFG>::count_used_entries(),
        4};
    out[1] = {
        CacheArena<CACHE8_CFG>::count_arenas(),
        CacheArena<CACHE8_CFG>::count_used_entries(),
        8};
    out[2] = {
        CacheArena<CACHE16_CFG>::count_arenas(),
        CacheArena<CACHE16_CFG>::count_used_entries(),
        16};
    out[3] = {
        CacheArena<CACHE32_CFG>::count_arenas(),
        CacheArena<CACHE32_CFG>::count_used_entries(),
        32};
    out[4] = {
        CacheArena<CACHE64_CFG>::count_arenas(),
        CacheArena<CACHE64_CFG>::count_used_entries(),
        64};
    out[5] = {
        CacheArena<CACHE128_CFG>::count_arenas(),
        CacheArena<CACHE128_CFG>::count_used_entries(),
        128};
    out[6] = {
        CacheArena<CACHE256_CFG>::count_arenas(),
        CacheArena<CACHE256_CFG>::count_used_entries(),
        256};
    out[7] = {
        CacheArena<CACHE512_CFG>::count_arenas(),
        CacheArena<CACHE512_CFG>::count_used_entries(),
        512};
    out[8] = {
        CacheArena<CACHE1024_CFG>::count_arenas(),
        CacheArena<CACHE1024_CFG>::count_used_entries(),
        1024};
    out[9] = {
        CacheArena<CACHE2048_CFG>::count_arenas(),
        CacheArena<CACHE2048_CFG>::count_used_entries(),
        2048};
}
}
#endif
