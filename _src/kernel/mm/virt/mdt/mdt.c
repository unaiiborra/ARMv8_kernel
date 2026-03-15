#include "mdt.h"

#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/panic.h>
#include <lib/align.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <lib/stdmacros.h>
#include <stddef.h>
#include <stdint.h>

#include "../../malloc/internal/reserve_malloc.h"


#define BF_BITS BITFIELD_CAPACITY(mdt_bf)

static vmalloc_mdt_container* container_list;


static inline vuintptr_t vsign(vuintptr_t va)
{
    return va_sign_extend(va, MM_MMU_HI_BITS);
}


static inline void
init_container(vmalloc_mdt_container* c, vmalloc_mdt_container* prev)
{
    DEBUG_ASSERT(c);
    DEBUG_ASSERT((vuintptr_t)c % KPAGE_ALIGN == 0);

    c->hdr.next = NULL;
    c->hdr.prev = prev;

    for (size_t i = 0; i < PA_MDT_CONTAINER_NODES; i++)
        c->entries[i] = (vmalloc_pa_mdt) {0};

    for (size_t i = 0; i < PA_MDT_BF_COUNT; i++)
        c->reserved_entries[i] = 0;

    if (prev) {
        DEBUG_ASSERT(!prev->hdr.next);
        prev->hdr.next = c;
    }
}


void vmalloc_pa_mdt_init()
{
    container_list = (vmalloc_mdt_container*)
                         early_kalloc(KPAGE_SIZE, "vmalloc pa mdt", true, false)
                             .va;

    init_container(container_list, NULL);
}


static void pa_mdt_container_free(vmalloc_mdt_container* c)
{
    if (c == container_list)
        return;

    if (c->hdr.prev) {
        c->hdr.prev->hdr.next = c->hdr.next;
    }
#ifdef DEBUG
    else {
        PANIC(
            "pa_mdt_container_free: prev should only be NULL if the node is "
            "the container list, "
            "which cannot be freed");
    }
#endif

    if (c->hdr.next)
        c->hdr.next->hdr.prev = c->hdr.prev;

#ifdef DEBUG
    for (size_t i = 0; i < PA_MDT_BF_COUNT; i++)
        DEBUG_ASSERT(c->reserved_entries[i] == 0);
#endif

    raw_kfree(c);
}


static vmalloc_mdt_container* pa_mdt_container_new(vmalloc_mdt_container* prev)
{
    DEBUG_ASSERT(prev);
    DEBUG_ASSERT(prev->hdr.next == NULL);

    vuintptr_t va = reserve_malloc("vmalloc mdt container").va;

    DEBUG_ASSERT((va & (KPAGE_SIZE - 1)) == 0);

    vmalloc_mdt_container* c = (vmalloc_mdt_container*)va;

    init_container(c, prev);

    return c;
}


static vmalloc_pa_mdt* node_new()
{
    size_t i, j, k;
    vmalloc_mdt_container *c, *p;

    c = container_list;
    p = NULL;

find:
    while (c) {
        for (i = 0; i < PA_MDT_CONTAINER_NODES; i++) {
            j = i / BF_BITS;
            k = i % BF_BITS;

            if (!bitfield_get(c->reserved_entries[j], k)) {
                // found free node
                bitfield_set_high(c->reserved_entries[j], k);

                return &c->entries[i];
            }
        }

        p = c;
        c = c->hdr.next;
    }

    c = pa_mdt_container_new(p);

    goto find;
}

#ifdef DEBUG
static bool mdt_is_valid(rva_node* n)
{
    if (!n->mdt.pa_mdt.list)
        return true; // no pa assigned

    vmalloc_pa_mdt *c, *p;
    vuintptr_t start, end, cur_start, cur_end, prev_end;

    start = vsign(n->start);
    end = vsign(n->start + n->size);
    prev_end = end;

    if (!is_aligned(start, KPAGE_ALIGN))
        return false;

    if (!is_aligned(end, KPAGE_ALIGN))
        return false;

    c = n->mdt.pa_mdt.list;
    p = NULL;

    while (c) {
        cur_start = c->info.va;
        cur_end = c->info.va + (power_of2(c->info.order) * KPAGE_SIZE);

        if (cur_start < start)
            return false;

        if (cur_end > end)
            return false;

        if (p && cur_start < prev_end)
            return false;

        if (!is_aligned(cur_start, KPAGE_ALIGN))
            return false;

        if (!is_aligned(cur_end, KPAGE_ALIGN))
            return false;


        prev_end = cur_end;
        p = c;
        c = c->next;
    }

    return true;
}
#endif


void vmalloc_pa_mdt_push(rva_node* n, size_t o, puintptr_t pa, vuintptr_t va)
{
    vmalloc_pa_mdt *cur, *prev, *node;

    n->mdt.pa_mdt.count++;

    DEBUG_ASSERT(n);

    node = node_new();
    *node = (vmalloc_pa_mdt) {
        .next = NULL,
        .info =
            (vmalloc_pa_info) {
                .order = o,
                .pa = pa,
                .va = va,
            },
    };

    cur = n->mdt.pa_mdt.list;
    prev = NULL;

    // first node
    if (!cur || va < cur->info.va) {
        node->next = cur;
        n->mdt.pa_mdt.list = node;
        return;
    }

    // search pos by va
    while (cur && cur->info.va <= va) {
        prev = cur;
        cur = cur->next;
    }

    DEBUG_ASSERT(prev);
    node->next = cur;
    prev->next = node;

#ifdef DEBUG
    DEBUG_ASSERT(mdt_is_valid(n), "vmalloc_pa_mdt_push: pa mdt is not coherent");
#endif
}


void vmalloc_pa_mdt_free(rva_node* n)
{
    DEBUG_ASSERT(n);

    vmalloc_pa_mdt* cur = n->mdt.pa_mdt.list;

    while (cur) {
        vmalloc_mdt_container* c =
            (void*)align_down((vuintptr_t)cur, KPAGE_ALIGN);
        vmalloc_pa_mdt* next = cur->next;

        vmalloc_pa_mdt* base = &c->entries[0];

        size_t i = (size_t)(cur - base);

        DEBUG_ASSERT(&c->entries[i] == cur);
        DEBUG_ASSERT(
            bitfield_get(c->reserved_entries[i / BF_BITS], i % BF_BITS));

        bitfield_clear(c->reserved_entries[i / BF_BITS], i % BF_BITS);


        bool empty_container = true;
        for (size_t j = 0; j < PA_MDT_BF_COUNT; j++)
            if (c->reserved_entries[j] != 0) {
                empty_container = false;
                break;
            }

        if (empty_container)
            pa_mdt_container_free(c);

        cur = next;
    }

    n->mdt.pa_mdt.count = 0;
    n->mdt.pa_mdt.list = NULL;
}
