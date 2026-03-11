#include "vmalloc.h"

#include <kernel/io/stdio.h>
#include <kernel/io/term.h>
#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/panic.h>
#include <lib/align.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <lib/stdmacros.h>
#include <lib/unit/mem.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../mm_info.h"
#include "containers/containers.h"
#include "mdt/mdt.h"


typedef enum {
    KMAP_LIST, // reserves exactly the provided v address, if not free, panics
    DYNAMIC_LIST, // reserves any address bigger or equal than v
} vmalloc_lists;


static v_uintptr_t pop_fva(const vmalloc_lists l, size_t pages, p_uintptr_t p);
static v_uintptr_t reserve_from_fva_node(
    fva_node** first,
    fva_node* cur,
    fva_node* prev,
    v_uintptr_t va,
    size_t bytes);
static rva_node* push_rva(
    const vmalloc_lists l,
    size_t pages,
    v_uintptr_t start,
    const char* tag,
    vmalloc_cfg cfg);


static fva_node* fva_kmap_list;
static fva_node* fva_dynamic_list;

static rva_node* rva_kmap_list;
static rva_node* rva_dynamic_list;


static inline v_uintptr_t vunsign(v_uintptr_t va)
{
    return va_zero_extend(va, MM_MMU_HI_BITS);
}

static inline v_uintptr_t vsign(v_uintptr_t va)
{
    return va_sign_extend(va, MM_MMU_HI_BITS);
}


// the provided va must be unsigned
static inline vmalloc_lists list_from_va(v_uintptr_t uva)
{
    v_uintptr_t addr_space_end = vunsign(kpa_to_kva(mm_info_mm_addr_space()));

    return uva < addr_space_end ? KMAP_LIST : DYNAMIC_LIST;
}


void vmalloc_init()
{
    vmalloc_init_containers();
    vmalloc_pa_mdt_init();

    fva_kmap_list = get_new_fva_node();
    fva_dynamic_list = get_new_fva_node();

    *fva_kmap_list = (fva_node) {
        .next = NULL,
        .start = vunsign(kpa_to_kva(0x0)),
        .size = kpa_to_kva(mm_info_mm_addr_space()) - kpa_to_kva(0x0),
    };

    *fva_dynamic_list = (fva_node) {
        .next = NULL,
        .start = vunsign(kpa_to_kva(mm_info_mm_addr_space())),
        .size = vunsign(~(v_uintptr_t)0) -
                vunsign(kpa_to_kva(mm_info_mm_addr_space())) + 1,
    };

    rva_kmap_list = NULL;
    rva_dynamic_list = NULL;
}


v_uintptr_t vmalloc_update_memregs(const early_memreg* mregs, size_t n)
{
    ASSERT(fva_kmap_list && fva_dynamic_list, "vmalloc: not initialized");
    ASSERT(
        fva_kmap_list->next == NULL && fva_dynamic_list->next == NULL,
        "vmalloc: not initialized");

    v_uintptr_t va = 0;
    early_memreg mb = (early_memreg) {0};

    for (size_t i = 0; i < n; i++) {
        mb = mregs[i];

        if (mb.free)
            continue;

        va = pop_fva(KMAP_LIST, mb.pages, mb.addr);

        ASSERT(ptrs_are_kmapped(pv_ptr_new(mb.addr, vsign(va))));

        push_rva(
            KMAP_LIST,
            mb.pages,
            va,
            is_kva((uintptr_t)mb.tag) ? mb.tag : kpa_to_kva_pt(mb.tag),
            (vmalloc_cfg) {
                .kmap =
                    {
                        .use_kmap = true,
                        .kmap_pa = mb.addr,
                    },
                .assing_pa = true,
                .device_mem = mb.device_memory,
                .permanent = mb.permanent,
            });
    }


    // get last free start
    fva_node* c = fva_kmap_list;
    while (c && c->next)
        c = c->next;

    return vsign(c->start);
}


static v_uintptr_t reserve_from_fva_node(
    fva_node** list,
    fva_node* cur,
    fva_node* prev,
    v_uintptr_t va,
    size_t bytes)
{
    DEBUG_ASSERT(
        address_is_valid(va, MM_MMU_HI_BITS, false),
        "reserve_from_fva_node: only unsigned va supported to avoid overflow");

    v_uintptr_t cur_start = cur->start;
    v_uintptr_t cur_end = cur->start + cur->size;


    DEBUG_ASSERT(cur_start <= va && va + bytes <= cur_start + cur->size);


    // get full node
    if (va == cur_start && bytes == cur->size) {
        if (prev)
            prev->next = cur->next;
        else
            *list = cur->next;

        free_fva_node(cur);
        return va;
    }

    // get from start
    if (va == cur_start) {
        cur->start += bytes;
        cur->size -= bytes;
        return va;
    }

    // get from end
    if (va + bytes == cur_end) {
        cur->size = va - cur_start;
        return va;
    }

    // split (is in the middle)
    fva_node* new = get_new_fva_node();

    new->start = va + bytes;
    new->size = cur_end - (va + bytes);
    new->next = cur->next;

    cur->size = va - cur_start;
    cur->next = new;

    return va;
}


static const size_t PAGE_L3 = MEM_KiB * 4;
static const size_t PAGE_L2 = PAGE_L3 * 512;
static const size_t PAGE_L1 = PAGE_L2 * 512;


static inline bool dynamic_fits_page_aligned(
    const size_t PAGE_ALIGN,
    size_t bytes,
    v_uintptr_t cur_start,
    v_uintptr_t cur_end)
{
    v_uintptr_t aligned = align_up(cur_start, PAGE_ALIGN);

    if (aligned < cur_start)
        return false; // overflow

    if (aligned >= cur_end)
        return false; // out of range

    return aligned + bytes <= cur_end;
}


/// p: the requested physical address in case of KMAP_LIST, else ignored
static v_uintptr_t pop_fva(const vmalloc_lists l, size_t pages, p_uintptr_t p)
{
    fva_node** const list =
        (l == KMAP_LIST) ? &fva_kmap_list : &fva_dynamic_list;

    DEBUG_ASSERT(list, "vmalloc: no available free va");

    size_t bytes = pages * KPAGE_SIZE;
    fva_node* cur = *list;
    fva_node* prev = NULL;

    v_uintptr_t kmap_va =
        l == KMAP_LIST ? vunsign(kpa_to_kva(p)) : (v_uintptr_t)NULL;
    fva_node *fits_dynamic_unaligned = NULL,
             *fits_dynamic_unaligned_prev = NULL;
    size_t PAGE_ALIGN;
    if (bytes >= PAGE_L1)
        PAGE_ALIGN = PAGE_L1;
    else if (bytes >= PAGE_L2)
        PAGE_ALIGN = PAGE_L2;
    else
        PAGE_ALIGN = PAGE_L3;

    while (cur) {
        v_uintptr_t cur_start = cur->start;
        v_uintptr_t cur_size = cur->size;
        v_uintptr_t cur_end = cur_start + cur_size;


        if (l == KMAP_LIST) {
            if (cur_start <= kmap_va && cur_end >= kmap_va + bytes)
                return reserve_from_fva_node(list, cur, prev, kmap_va, bytes);
        }
        //  l == KMAP_DYNAMIC
        else {
            if (bytes <= cur_size) {
                if (cur_size >= PAGE_ALIGN && dynamic_fits_page_aligned(
                                                  PAGE_ALIGN,
                                                  bytes,
                                                  cur_start,
                                                  cur_end)) {
                    return reserve_from_fva_node(
                        list,
                        cur,
                        prev,
                        align_up(cur_start, PAGE_ALIGN),
                        bytes);
                }

                if (!fits_dynamic_unaligned) {
                    fits_dynamic_unaligned = cur;
                    fits_dynamic_unaligned_prev = prev;
                }
            }
        }

        prev = cur;
        cur = cur->next;
    }

    if (l == DYNAMIC_LIST && fits_dynamic_unaligned) {
        return reserve_from_fva_node(
            list,
            fits_dynamic_unaligned,
            fits_dynamic_unaligned_prev,
            fits_dynamic_unaligned->start,
            bytes);
    }

    PANIC("vmalloc: no available free virtual address range found");
}


static void push_fva(const vmalloc_lists l, v_uintptr_t va, size_t bytes)
{
    fva_node** const list =
        (l == KMAP_LIST) ? &fva_kmap_list : &fva_dynamic_list;
    fva_node* cur = *list;
    fva_node* prev = NULL;


    while (cur && cur->start < va) {
        prev = cur;
        cur = cur->next;
    }

#ifdef DEBUG
    if (prev)
        ASSERT(prev->start + prev->size <= va);

    if (cur)
        ASSERT(va + bytes <= cur->start);
#endif

    // if it can be joined with the prev node
    if (prev && prev->start + prev->size == va) {
        prev->size += bytes;

        // watch if it can now be joined with the next node
        if (cur && va + bytes == cur->start) {
            prev->size += cur->size;
            prev->next = cur->next;
            free_fva_node(cur);
        }

        return;
    }

    // if it can be joined with the next one
    if (cur && va + bytes == cur->start) {
        cur->start = va;
        cur->size += bytes;
        return;
    }


    // cannot be joined, alloc a new node
    fva_node* node = get_new_fva_node();

    *node = (fva_node) {
        .next = cur,
        .start = va,
        .size = bytes,
    };

    if (prev)
        prev->next = node;
    else
        *list = node;
}


static bool find_rva(v_uintptr_t start, rva_node** node, rva_node** prev)
{
    start = is_kva_uintptr_t(start) ? vunsign(start) : start;

    const vmalloc_lists l = list_from_va(start);

    rva_node* c = (l == KMAP_LIST) ? rva_kmap_list : rva_dynamic_list;
    rva_node* p = NULL;

    if (!c)
        return false;

    while (node) {
        if (c->start == start) {
            if (node)
                *node = c;
            if (prev)
                *prev = p;

            return true;
        }

        p = c;
        c = c->next;
    }

    return false;
}


// returns the size in bytes of the reserved va
static size_t pop_rva(
    const vmalloc_lists l,
    v_uintptr_t start,
    vmalloc_allocated_area_mdt* info)
{
    rva_node *node, *prev;

    bool found = find_rva(start, &node, &prev);

    ASSERT(found, "vmalloc: attempted to free an unreserved virtual address");
    ASSERT(!node->mdt.info.permanent, "vmalloc: cannot free a permanent va");

    size_t size = node->size;

    if (info)
        *info = node->mdt.info;

    if (prev)
        prev->next = node->next;
    else
        *((l == KMAP_LIST) ? &rva_kmap_list : &rva_dynamic_list) = node->next;


    vmalloc_pa_mdt_free(node);
    free_rva_node(node);

    return size;
}


static rva_node* push_rva(
    const vmalloc_lists l,
    size_t pages,
    v_uintptr_t start,
    const char* tag,
    vmalloc_cfg cfg)
{
    DEBUG_ASSERT(
        (cfg.kmap.use_kmap && l == KMAP_LIST) ||
        (!cfg.kmap.use_kmap && l == DYNAMIC_LIST));


    size_t bytes = pages * KPAGE_SIZE;

    rva_node* cur = (l == KMAP_LIST) ? rva_kmap_list : rva_dynamic_list;
    rva_node* prev = NULL;

    while (cur && cur->start < start) {
        prev = cur;
        cur = cur->next;
    }

#ifdef DEBUG
    if (prev)
        DEBUG_ASSERT(prev->start + prev->size <= start);

    if (cur)
        DEBUG_ASSERT(start + bytes <= cur->start);
#endif

    rva_node* node = get_new_rva_node();

    *node = (rva_node) {
        .next = cur,
        .start = start,
        .size = bytes,
        .mdt =
            {
                .info =
                    {
                        .tag = tag,
                        .kmapped = cfg.kmap.use_kmap,
                        .pa_assigned = cfg.assing_pa,
                        .device_mem = cfg.device_mem,
                        .permanent = cfg.permanent,
                    },
                .pa_mdt =
                    {
                        .count = 0,
                        .list = NULL,
                    },
            },
    };

    if (prev)
        prev->next = node;
    else
        *((l == KMAP_LIST) ? &rva_kmap_list : &rva_dynamic_list) = node;

    return node;
}


v_uintptr_t
vmalloc(size_t pages, const char* tag, vmalloc_cfg cfg, vmalloc_token* t)
{
    vmalloc_lists l = cfg.kmap.use_kmap ? KMAP_LIST : DYNAMIC_LIST;


    if (cfg.kmap.use_kmap) {
        ASSERT(
            is_pow2(pages),
            "vmalloc: only pow2 n pages can be requested for kmapping");
        ASSERT(
            cfg.assing_pa,
            "vmalloc: kmap can only be configured with assign_pa = true");
    }


    v_uintptr_t start = pop_fva(l, pages, cfg.kmap.kmap_pa);

#ifdef DEBUG
    if (cfg.kmap.use_kmap)
        DEBUG_ASSERT(start == vunsign(kpa_to_kva(cfg.kmap.kmap_pa)));
    else
        DEBUG_ASSERT(start >= vunsign(kpa_to_kva(mm_info_mm_addr_space())));
#endif

    rva_node* n = push_rva(l, pages, start, tag, cfg);

    if (t)
        t->rva_ = n;

    return vsign(start);
}


size_t vmalloc_get_free_bytes()
{
    size_t b = 0;

    fva_node* lists[2] = {fva_kmap_list, fva_dynamic_list};
    fva_node* cur;

    for (size_t i = 0; i < 2; i++) {
        cur = lists[i];

        while (cur) {
            b += cur->size;
            cur = cur->next;
        }
    }

    return b;
}


size_t vmalloc_get_reserved_bytes()
{
    size_t b = 0;

    rva_node* lists[2] = {rva_kmap_list, rva_dynamic_list};
    rva_node* cur;

    for (size_t i = 0; i < 2; i++) {
        cur = lists[i];

        while (cur) {
            b += cur->size;
            cur = cur->next;
        }
    }

    return b;
}


const char* vmalloc_update_tag(v_uintptr_t addr, const char* new_tag)
{
    ASSERT(addr);

    rva_node *node, *prev;

    v_uintptr_t va = vunsign(addr);

    bool found = find_rva(va, &node, &prev);
    ASSERT(found, "vmalloc_update_tag: requested va not allocated");

    const char* old_tag = node->mdt.info.tag;
    node->mdt.info.tag = new_tag;

    return old_tag;
}


vmalloc_va_info vmalloc_get_addr_info(void* addr)
{
    v_uintptr_t a = vunsign((v_uintptr_t)addr);
    vmalloc_lists l = list_from_va(a);


    fva_node* fcur = l == KMAP_LIST ? fva_kmap_list : fva_dynamic_list;

    while (fcur) {
        v_uintptr_t start = fcur->start;
        v_uintptr_t end = start + fcur->size;

        if (start > a)
            break;

        if (end > a) {
            return (vmalloc_va_info) {.state = VMALLOC_VA_INFO_FREE,
                                      .state_info = {
                                          .free = {
                                              .free_start = vsign(start),
                                              .free_size = fcur->size,
                                          }}};
        }

        fcur = fcur->next;
    }


    rva_node* rcur = l == KMAP_LIST ? rva_kmap_list : rva_dynamic_list;

    while (rcur) {
        v_uintptr_t start = rcur->start;
        v_uintptr_t end = start + rcur->size;

        if (start > a)
            break;

        if (end > a) {
            return (vmalloc_va_info) {.state = VMALLOC_VA_INFO_RESERVED,
                                      .state_info = {
                                          .reserved = {
                                              .reserved_start = vsign(start),
                                              .reserved_size = rcur->size,
                                              .mdt = rcur->mdt,
                                          }}};
        }

        rcur = rcur->next;
    }

    return (vmalloc_va_info) {.state = VMALLOC_VA_UNREGISTERED};
}


vmalloc_token vmalloc_get_token(void* allocation_addr)
{
    rva_node* n;
    bool result = find_rva((v_uintptr_t)allocation_addr, &n, NULL);

    ASSERT(result, "vmalloc_get_token: token not found");

    return (vmalloc_token) {.rva_ = n};
}

#ifdef DEBUG
static void validate_vmalloc_token(vmalloc_token t)
{
    DEBUG_ASSERT(t.rva_);
    rva_node* lists[2] = {rva_kmap_list, rva_dynamic_list};

    for (size_t i = 0; i < 2; i++) {
        rva_node* cur = lists[i];
        while (cur) {
            if (cur == (rva_node*)t.rva_) {
                vmalloc_container* c =
                    (vmalloc_container*)align_down((uintptr_t)cur, KPAGE_ALIGN);
                rva_node* base = &c->rva.data.nodes[0];

#    define T typeof(c->rva.data.reserved_nodes[0])
                size_t i, j, k;
                i = (size_t)(cur - base);
                j = i / BITFIELD_CAPACITY(T);
                k = i % BITFIELD_CAPACITY(T);

                DEBUG_ASSERT(bitfield_get(c->rva.data.reserved_nodes[j], k));

                return;
            }
            cur = cur->next;
        }
    }
    PANIC(
        "vmalloc_push_pa: provided a token with an inexistent node address, it "
        "might have been "
        "freed before");
}

#    undef T

#else
#    define validate_vmalloc_token(t)
#endif


vmalloc_allocated_area_mdt __vmalloc_token__vmalloc_get_mdt(vmalloc_token t)
{
    validate_vmalloc_token(t);

    rva_node* rva = t.rva_;

    return rva->mdt.info;
}

vmalloc_allocated_area_mdt __voidptr__vmalloc_get_mdt(void* allocation_addr)
{
    vmalloc_token t = vmalloc_get_token(allocation_addr);

    return __vmalloc_token__vmalloc_get_mdt(t);
}


void vmalloc_push_pa(
    vmalloc_token t,
    size_t order,
    p_uintptr_t pa,
    v_uintptr_t va)
{
    validate_vmalloc_token(t);

    rva_node* n = t.rva_;

    vmalloc_pa_mdt_push(n, order, pa, va);
}


size_t vmalloc_get_pa_count(vmalloc_token t)
{
    validate_vmalloc_token(t);

    rva_node* n = t.rva_;

    return n->mdt.pa_mdt.count;
}


bool vmalloc_get_pa_info(vmalloc_token t, vmalloc_pa_info* buf, size_t buf_size)
{
    validate_vmalloc_token(t);

    rva_node* n = t.rva_;

    size_t count = n->mdt.pa_mdt.count;

    if (buf_size < count)
        return false;

    vmalloc_pa_mdt* cur = n->mdt.pa_mdt.list;

    size_t i = 0;

    while (cur) {
        if (i >= count)
            return false;

        buf[i++] = cur->info;

        cur = cur->next;
    }

    return true;
}


size_t __vmalloc_token__vfree(vmalloc_token t, vmalloc_allocated_area_mdt* info)
{
    validate_vmalloc_token(t);

    rva_node* rva = t.rva_;

    v_uintptr_t va = rva->start;

    vmalloc_lists l = list_from_va(va);

    size_t bytes = pop_rva(l, va, info);
    push_fva(l, va, bytes);

    return bytes;
}


size_t __voidptr__vfree(void* va, vmalloc_allocated_area_mdt* info)
{
    vmalloc_token t = vmalloc_get_token(va);

    return __vmalloc_token__vfree(t, info);
}


void vmalloc_debug_free()
{
    size_t va_bits = MM_MMU_HI_BITS;
    v_uintptr_t mask = (1ULL << va_bits) - 1;

    kprint("\n\r");
    kprint("=============================================\n\r");
    kprint("           VMALLOC - FREE REGIONS            \n\r");
    kprint("=============================================\n\r");
    kprint("   START              END                SIZE (bytes / pages)\n\r");
    kprint("-------------------------------------------------------------\n\r");
    kprint("[K == kmapped, - == dynamic]\n\r");

    size_t total = 0;
    fva_node* lists[2] = {fva_kmap_list, fva_dynamic_list};

    for (size_t i = 0; i < 2; i++) {
        fva_node* cur = lists[i];
        char k = i == KMAP_LIST ? 'K' : '-'; // kmapped char

        while (cur) {
            v_uintptr_t start = cur->start;
            v_uintptr_t end = start + cur->size;

            if ((end & ~mask) != 0) // overflow
                end = mask;

            start = vsign(start);
            end = vsign(end);

            size_t pages = cur->size / KPAGE_SIZE;

            kprintf(
                "   [%p - %p)   %p B  (%p pages)    [%c]\n\r",
                start,
                end,
                cur->size,
                pages,
                k);

            total += cur->size;
            cur = cur->next;
        }
    }

    kprint("-------------------------------------------------------------\n\r");
    kprintf("   TOTAL FREE: %p bytes (%p pages)\n\r", total, total / KPAGE_SIZE);
    kprint("=============================================\n\r");
}


void vmalloc_debug_reserved()
{
    size_t va_bits = MM_MMU_HI_BITS;
    v_uintptr_t mask = (1ULL << va_bits) - 1;

    kprint("\n\r");
    kprint("=============================================\n\r");
    kprint("         VMALLOC - RESERVED REGIONS\n\r");
    kprint("=============================================\n\r");
    kprint("   START              END                SIZE    FLAGS\n\r");
    kprint("-------------------------------------------------------------\n\r");

    size_t total = 0;
    rva_node* lists[2] = {rva_kmap_list, rva_dynamic_list};

    for (size_t i = 0; i < 2; i++) {
        rva_node* cur = lists[i];

        while (cur) {
            v_uintptr_t start = cur->start;
            v_uintptr_t end = start + cur->size;

            if ((end & ~mask) != 0) // overflow
                end = mask;

            start = vsign(start);
            end = vsign(end);

            size_t pages = cur->size / KPAGE_SIZE;

            kprintf(
                "   [%p - %p)   %p B (%d p)   ",
                start,
                end,
                cur->size,
                pages);
            kprintf("[%c", cur->mdt.info.kmapped ? 'K' : '-');
            kprintf("%c", cur->mdt.info.pa_assigned ? 'P' : '-');
            kprintf("%c", cur->mdt.info.device_mem ? 'D' : '-');
            kprintf("%c]", cur->mdt.info.permanent ? '!' : '-');

            kprintf("\t%s\n\r", cur->mdt.info.tag);

            total += cur->size;
            cur = cur->next;
        }
    }

    kprint("-------------------------------------------------------------\n\r");
    kprintf(
        "   TOTAL RESERVED: %p bytes (%p pages)\n\r",
        total,
        total / KPAGE_SIZE);
    kprint("=============================================\n\r");
}


void vmalloc_debug_nodes()
{
    kprint("\n\r");
    kprint("=============================================\n\r");
    kprint("           VMALLOC - FVA NODES\n\r");
    kprint("=============================================\n\r");

    fva_node* fva[2] = {fva_kmap_list, fva_dynamic_list};

    kprint("--- FVA CONTAINERS ---\n\r");
    vmalloc_containers_debug_fva();

    for (size_t i = 0; i < 2; i++) {
        (i == 0) ? kprint("\n\r--- FVA KMAP LIST ---\n\r")
                 : kprint("\n\r--- FVA DYNAMIC LIST ---\n\r");

        fva_node* cur = fva[i];
        size_t j = 0;
        while (cur) {
            kprintf("node[%d] %p, %d bytes\n\r", j++, cur->start, cur->size);

            cur = cur->next;
        }
    }


    kprint("\n\r");
    kprint("=============================================\n\r");
    kprint("           VMALLOC - RVA NODES\n\r");
    kprint("=============================================\n\r");

    kprint("---RVA CONTAINERS---\n\r");
    vmalloc_containers_debug_rva();

    rva_node* rva[2] = {rva_kmap_list, rva_dynamic_list};

    for (size_t i = 0; i < 2; i++) {
        (i == 0) ? kprint("\n\r--- RVA KMAP LIST ---\n\r")
                 : kprint("\n\r--- RVA DYNAMIC LIST ---\n\r");

        rva_node* cur = rva[i];
        size_t j = 0;
        while (cur) {
            kprintf(
                "node[%d %s] %p, %d bytes\n\r",
                j++,
                cur->mdt.info.tag,
                cur->start,
                cur->size);

            cur = cur->next;
        }
    }
}
