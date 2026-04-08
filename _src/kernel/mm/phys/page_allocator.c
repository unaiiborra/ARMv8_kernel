#include "page_allocator.h"

#include <arm/mmu.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/align.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdmacros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../malloc/raw_kmalloc/raw_kmalloc.h"
#include "../mm_info.h"


typedef uint32_t node_data;

#define ORDER_SHIFT 0
#define ORDER_BITS  7

#define FREE_SHIFT 8
#define FREE_BITS  1

#define UNSHIFTED_MASK(bit_n) ((1U << (bit_n)) - 1)
#define MASK(bit_n, shift)    (UNSHIFTED_MASK(bit_n) << shift)


#define NULL_IDX       ((uint32_t)~0)
#define IS_NULL_IDX(i) (i == NULL_IDX)

typedef struct page_node {
    uint32_t     next;
    uint32_t     prev;
    node_data    node_data;
    mm_page_data page_data;
} page_node;


static inline uint32_t buddy_of(uint32_t i, uint8_t o);
static inline uint32_t parent_at_order(uint32_t i, uint8_t target_o);

static inline uint8_t get_order(page_node* n);
static inline void    set_order(page_node* n, uint8_t o);

static inline bool get_free(page_node* n);
static inline void set_free(page_node* n, bool v);

static inline page_node* get_node(uint32_t i);

static inline bool is_in_order_free_list(uint32_t i, uint8_t o);
#ifdef DEBUG
static bool is_in_free_list(uint32_t i);
#endif
static bool is_inner_idx(uint32_t i);

static void push_to_list(uint32_t i);
static void remove_from_list(uint32_t i);

static void split_to_order_and_pop(uint32_t i, uint8_t target_o);
static void try_merge(uint32_t i);


static size_t     N;
static size_t     MAX_ORDER;
static page_node* nodes;
static uint32_t*  free_lists;


static inline uint32_t buddy_of(uint32_t i, uint8_t o)
{
    return i ^ (1U << o);
}


static inline uint32_t parent_at_order(uint32_t i, uint8_t target_o)
{
    return i & ~((1U << target_o) - 1);
}


static inline uint8_t get_order(page_node* n)
{
    return (uint8_t)((n->node_data >> ORDER_SHIFT) &
                     UNSHIFTED_MASK(ORDER_BITS));
}


static inline void set_order(page_node* n, uint8_t o)
{
    DEBUG_ASSERT((o & (1U << ORDER_BITS)) == 0, "set_order: order uses 7 bits");

    n->node_data &= ~MASK(ORDER_BITS, ORDER_SHIFT);
    n->node_data |= o << ORDER_SHIFT;
}


static inline bool get_free(page_node* n)
{
    bool res = ((n->node_data >> FREE_SHIFT) & UNSHIFTED_MASK(FREE_BITS)) != 0;

    DEBUG_ASSERT(res == is_in_free_list((uint32_t)(n - nodes)));

    return res;
}

static inline void set_free(page_node* n, bool v)
{
    DEBUG_ASSERT(v == !!v);

    n->node_data &= ~MASK(FREE_BITS, FREE_SHIFT);
    n->node_data |= (node_data)v << FREE_SHIFT;
}


static inline page_node* get_node(uint32_t i)
{
    DEBUG_ASSERT(i < N);
    DEBUG_ASSERT(i != NULL_IDX);

    return &nodes[i];
}

static inline bool is_in_order_free_list(uint32_t i, uint8_t o)
{
    page_node* n = get_node(i);
    uint32_t   j = free_lists[o];

    if (IS_NULL_IDX(j))
        return false;

    loop
    {
        if (i == j)
            return true;

        n = get_node(j);

        if (!IS_NULL_IDX(n->next))
            j = n->next;
        else
            return false;
    }
}

static bool is_in_free_list(uint32_t i)
{
    return is_in_order_free_list(i, get_order(get_node(i)));
}


static bool is_inner_idx(uint32_t i)
{
    DEBUG_ASSERT(i < N);

    page_node* n     = get_node(i);
    uint8_t    order = get_order(n);

    for (size_t target_o = order + 1; target_o <= MAX_ORDER; target_o++) {
        i = parent_at_order(i, target_o);

        if (is_in_order_free_list(i, target_o)) {
            DEBUG_ASSERT(get_free(get_node(i)));

            return true;
        }
    }

    return false;
}


static void push_to_list(uint32_t i)
{
    page_node* n = get_node(i);
    uint8_t    o = get_order(n);

    DEBUG_ASSERT(o <= MAX_ORDER);
    DEBUG_ASSERT(IS_NULL_IDX(n->prev));
    DEBUG_ASSERT(IS_NULL_IDX(n->next));

    uint32_t head = free_lists[o];

    n->prev = NULL_IDX;
    n->next = head;

    if (!IS_NULL_IDX(n->next))
        get_node(n->next)->prev = i;

    free_lists[o] = i;
    set_free(n, true);
}


static void remove_from_list(uint32_t i)
{
    page_node* n = get_node(i);
    uint8_t    o = get_order(n);

    DEBUG_ASSERT(o <= MAX_ORDER);
    DEBUG_ASSERT(get_free(n));

    if (IS_NULL_IDX(n->prev)) {
        DEBUG_ASSERT(free_lists[o] == i);

        free_lists[o] = n->next;

        if (!IS_NULL_IDX(n->next))
            get_node(n->next)->prev = NULL_IDX;
    }
    else {
        get_node(n->prev)->next = n->next;

        if (!IS_NULL_IDX(n->next))
            get_node(n->next)->prev = n->prev;
    }

    n->next = NULL_IDX;
    n->prev = NULL_IDX;
    set_free(n, false);
}


static void split_to_order_and_pop(uint32_t i, uint8_t target_o)
{
    DEBUG_ASSERT(target_o < MAX_ORDER);

    page_node* n = get_node(i);
    DEBUG_ASSERT(get_free(n));

    uint8_t o = get_order(n);

    ASSERT(o > target_o);
    DEBUG_ASSERT(o > 0 && o <= MAX_ORDER);

    remove_from_list(i);
    set_order(n, target_o);

    while (o-- > target_o) {
        int32_t    b  = buddy_of(i, o);
        page_node* bn = get_node(b);

        set_order(bn, o);
        push_to_list(b);
    }
}


static void try_merge(uint32_t i)
{
    loop
    {
        page_node* n = get_node(i);
        uint8_t    o = get_order(n);

        if (o == MAX_ORDER)
            return;

        uint32_t   b     = buddy_of(i, o);
        page_node* buddy = get_node(b);

        if (!get_free(buddy) || get_order(buddy) != o)
            return;

        remove_from_list(i);
        remove_from_list(b);

        uint32_t left = (i < b) ? i : b;
        set_order(get_node(left), o + 1);
        push_to_list(left);

        i = left;
    }
}


puintptr_t page_malloc(uint8_t order, mm_page_data p)
{
    DEBUG_ASSERT(order <= MAX_ORDER);

    page_node* n;
    uint32_t   i = free_lists[order];

    if (IS_NULL_IDX(i)) {
        for (uint8_t o = order; o <= MAX_ORDER; o++) {
            i = free_lists[o];

            if (!IS_NULL_IDX(i)) {
                split_to_order_and_pop(i, order);
                n            = get_node(i);
                n->page_data = p;
                return i * PAGE_SIZE;
            }
        }

        PANIC("page_malloc: no free pages for the requested or bigger order");
    }

    n = get_node(i);
    remove_from_list(i);

    n->page_data = p;

    return i * PAGE_SIZE;
}


void page_free(puintptr_t pa)
{
    uint32_t   i = pa / PAGE_SIZE;
    page_node* n = get_node(i);


    if (n->page_data.permanent)
        PANIC("page_free: cannot free a permanent region");

    if (get_free(n))
        PANIC("page_free: double free");

    if (is_inner_idx(i))
        PANIC("page_free: invalid pa provided");

    push_to_list(i);
    try_merge(i);
}


const char* page_allocator_update_tag(puintptr_t pa, const char* new_tag)
{
    uint32_t   i = pa / PAGE_SIZE;
    page_node* n = get_node(i);

    ASSERT(!is_inner_idx(i));
    ASSERT(!is_kva(pa));
    ASSERT(is_kva_ptr(new_tag));

    const char* old  = n->page_data.tag;
    n->page_data.tag = new_tag;
    return old;
}


bool page_allocator_get_data(puintptr_t pa, mm_page_data* data)
{
    uint32_t   i = pa / PAGE_SIZE;
    page_node* n = get_node(i);


    raw_kmalloc_lock();
    __attribute__((cleanup(raw_kmalloc_unlock))) int __defer
        __attribute__((unused));


    if (is_inner_idx(i))
        return false;

    if (is_in_free_list(get_order(n)))
        return false;

    *data = n->page_data;

    return true;
}


bool page_allocator_set_data(puintptr_t pa, mm_page_data data)
{
    uint32_t   i = pa / PAGE_SIZE;
    page_node* n = get_node(i);

    raw_kmalloc_lock();
    __attribute__((cleanup(raw_kmalloc_unlock))) int __defer
        __attribute__((unused));


    if (is_inner_idx(i))
        return false;

    if (is_in_free_list(get_order(n)))
        return false;


    n->page_data = data;

    return true;
}


static void reserve(uint32_t i, uint8_t o)
{
    ASSERT((i & ((1U << o) - 1)) == 0);

    uint8_t  k;
    uint32_t base;

    for (k = o; k <= MAX_ORDER; k++) {
        base         = parent_at_order(i, k);
        page_node* p = get_node(base);

        if (get_free(p) && get_order(p) == k)
            break;
    }

    if (k > MAX_ORDER)
        PANIC("double reservation");

    remove_from_list(base);

    uint32_t cur   = base;
    uint8_t  cur_o = k;

    while (cur_o > o) {
        cur_o--;

        uint32_t left  = cur;
        uint32_t right = cur + (1U << cur_o);

        if (i < right) {
            set_order(get_node(right), cur_o);
            push_to_list(right);
            cur = left;
        }
        else {
            set_order(get_node(left), cur_o);
            push_to_list(left);
            cur = right;
        }
    }

    set_order(get_node(cur), o);
}

void page_allocator_init()
{
    N         = mm_info_page_count();
    MAX_ORDER = log2_floor(N);

    size_t free_list_bytes =
        align_up(sizeof(uint32_t*) * MAX_ORDER, _Alignof(page_node));
    size_t nodes_bytes = sizeof(page_node) * N;


    pv_ptr pv = early_kalloc(
        align_up(free_list_bytes + nodes_bytes, PAGE_SIZE),
        "page allocator",
        true,
        false);

    free_lists = (uint32_t*)pv.va;
    nodes      = (page_node*)(pv.va + free_list_bytes);

    ASSERT((vuintptr_t)free_lists % _Alignof(uint32_t) == 0);
    ASSERT((vuintptr_t)nodes % _Alignof(page_node) == 0);


    size_t i;
    for (i = 0; i <= MAX_ORDER; i++)
        free_lists[i] = NULL_IDX;

    for (i = 0; i < N; i++) {
        *get_node(i) = (page_node) {
            .next      = NULL_IDX,
            .prev      = NULL_IDX,
            .node_data = 0,
            .page_data = {0},
        };
    }

    i                = 0;
    size_t remaining = N;
    while (remaining) {
        size_t o = log2_floor(remaining);

        page_node* n = get_node(i);
        set_order(n, o);
        push_to_list(i);

        i += power_of2(o);
        remaining -= power_of2(o);
    }
}


void page_allocator_update_memregs(const early_memreg* mregs, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        early_memreg e = mregs[i];

        DEBUG_ASSERT(is_kva_ptr(e.tag));

        size_t pages     = e.pages;
        size_t remaining = pages;
        size_t j         = e.addr / PAGE_SIZE;


        while (remaining > 0) {
            size_t o = log2_floor(remaining);

            if (e.free) {
                j += power_of2(o);
                break;
            }

            while ((j & ((1U << o) - 1)) != 0)
                o--;

            reserve(j, o);
            get_node(j)->page_data = (mm_page_data) {
                .tag        = e.tag,
                .permanent  = e.permanent,
                .device_mem = e.device_memory,
            };

            remaining -= power_of2(o);
            j += power_of2(o);
        }
    }
}


void page_allocator_debug()
{
    size_t i = 0;

    puintptr_t addr;
    size_t     bytes, pages;

    while (i < N) {
        page_node* n = get_node(i);

        addr  = i * PAGE_SIZE;
        pages = power_of2(get_order(n));
        bytes = pages * PAGE_SIZE;


        if (get_free(n)) {
            kprintf(
                "\t[F%d-%p] %dp, %p bytes\n\r",
                get_order(n),
                addr,
                pages,
                bytes);
        }
        else {
            mm_page_data d = n->page_data;

            kprintf(
                "\t[R%d|%s|%p][%s%s] %dp, %p bytes \n\r",
                get_order(n),
                d.tag,
                addr,
                d.permanent ? "!" : "-",
                d.device_mem ? "MMIO" : "RAM",
                pages,
                bytes);
        }

        i += pages;
    }
}
