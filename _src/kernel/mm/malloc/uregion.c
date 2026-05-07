#include <kernel/mm.h>
#include <kernel/mm/uregion.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/mmu.h"
#include "kernel/lib/kvec.h"
#include "kernel/mm/cache_malloc.h"
#include "kernel/mm/mmu.h"
#include "kernel/mm/page_malloc.h"
#include "kernel/mm/vmalloc.h"
#include "kernel/panic.h"
#include "kernel/task.h"
#include "lib/align.h"
#include "lib/branch.h"
#include "lib/flags.h"
#include "lib/math.h"
#include "lib/mem.h"
#include "lib/stdbitfield.h"
#include "raw_kmalloc/raw_kmalloc.h"





/// flags of the entire region. It is uregion_flags_e + internal flags
typedef enum {
    F_READ  = UREGION_F_READ,
    F_WRITE = UREGION_F_WRITE,
    F_EXEC  = UREGION_F_EXEC,
    F_STATIC_LIFETIME,
    F_FULL_MAPPED,
    F_PARTIALLY_MAPPED,
    F_HAS_KNL,
} uregion_flags_internal_e;


typedef struct uregion {
    uintptr_t usr_start; // userspace va start
    uint32_t  pages;     // page count
    uint32_t  flags;     // uregion_flags_internal_e

    // kernel access
    uintptr_t     knl_start;  // kernel space va start. 0 if not assigned.
    vmalloc_token knl_vtoken; // vmalloc token for knl allocation. Invalid if
                              // knl_start == 0

    // bitfield of pages with a physical address assigned
    // if pages <= 64: sm
    // si pages >  64: bg (allocated array)
    union {
        bitfield64  sm;
        bitfield64* bg;
    } committed;
} uregion_t;

typedef struct uregion_node {
    struct uregion_node* next;
    uregion_t            region;
} uregion_node_t;


[[gnu::always_inline]] static inline uregion_node_t* get_head_node(task* t)
{
    if (!t->regions)
        return NULL;

    return (uregion_node_t*)((uint8_t*)t->regions -
                             offsetof(uregion_node_t, region));
}

static void commited_try_init(uregion_t* region)
{
    size_t pages = region->pages;

    bool initialized = pages <= 64 || region->committed.bg != NULL;

    if (likely(initialized))
        return;

    region->committed.bg = kmalloc(
        div_ceil(pages, BITFIELD_CAPACITY(bitfield64)) * sizeof(bitfield64));
}


static inline void committed_set(
    uregion_t* region,
    size_t     page_idx, // relative to the region
    size_t     count)
{
    constexpr size_t N = BITFIELD_CAPACITY(typeof(region->committed.sm));

    bitfield64* bf = region->pages > N ? region->committed.bg
                                       : &region->committed.sm;

    for (size_t i = 0; i < count; i++) {
        size_t idx  = page_idx + i;
        size_t bf_n = idx / N;
        size_t bf_i = idx % N;
        DEBUG_ASSERT(region->pages <= N ? bf_n == 0 : true);
        bitfield_set_high(bf[bf_n], bf_i);
    }
}


static mmu_pg_cfg usr_mmu_cfg_from_flags(uint32_t flags)
{
    constexpr mmu_pg_cfg RW = (mmu_pg_cfg) {
        0,
        MMU_AP_EL0_RW_EL1_RW,
        MMU_SH_INNER_SHAREABLE,
        false,
        true,
        true,
        true,
        0,
    };

    constexpr mmu_pg_cfg R = (mmu_pg_cfg) {
        0,
        MMU_AP_EL0_RO_EL1_RO,
        MMU_SH_INNER_SHAREABLE,
        false,
        true,
        true,
        true,
        0,
    };

    constexpr mmu_pg_cfg RX = (mmu_pg_cfg) {
        0,
        MMU_AP_EL0_RO_EL1_RO,
        MMU_SH_INNER_SHAREABLE,
        false,
        true,
        true,
        false,
        0,
    };


    switch (flags & 0b111U) {
        case FLAG_BIT(F_READ) | FLAG_BIT(F_WRITE):
            return RW;

        case FLAG_BIT(F_READ):
            return R;

        case FLAG_BIT(F_READ) | FLAG_BIT(F_EXEC):
            return RX;

        default:
            PANIC("mmu_cfg_from_flags: invalid flags");
    }
}


// finds the node whose region contains the usr va
static uregion_node_t* region_find_usrva(task* t, uintptr_t usrva)
{
    uregion_node_t* curr = get_head_node(t);

    while (curr) {
        uintptr_t start = curr->region.usr_start;
        uintptr_t end   = start + curr->region.pages * PAGE_SIZE;

        if (usrva >= start && usrva < end)
            return curr;

        curr = curr->next;
    }

    return NULL;
}

// ordered by usr_start, returns false if it overlaps (and does not insert)
static bool region_insert(task* t, uregion_node_t* node)
{
    uintptr_t new_start = node->region.usr_start;
    uintptr_t new_end   = new_start + node->region.pages * PAGE_SIZE;

    uregion_node_t* prev = NULL;
    uregion_node_t* curr = get_head_node(t);

    // ordered high to low
    while (curr && curr->region.usr_start > new_start) {
        prev = curr;
        curr = curr->next;
    }

    // check overlap with prev (which has higher address)
    if (prev) {
        uintptr_t prev_start = prev->region.usr_start;
        if (new_end > prev_start)
            return false;
    }

    // check overlap with curr (which has lower address)
    if (curr) {
        uintptr_t curr_end = curr->region.usr_start +
                             curr->region.pages * PAGE_SIZE;
        if (curr_end > new_start)
            return false;
    }

    node->next = curr;

    if (prev)
        prev->next = node;
    else
        t->regions = &node->region;

    return true;
}

static bool region_remove(task* t, uregion_node_t* node)
{
    uregion_node_t* prev = NULL;
    uregion_node_t* curr = get_head_node(t);

    while (curr) {
        if (curr == node) {
            if (prev)
                prev->next = curr->next;
            else
                t->regions = curr->next ? &curr->next->region : NULL;
            return true;
        }

        prev = curr;
        curr = curr->next;
    }

    return false;
}

static uregion_node_t* node_malloc(
    uintptr_t usr_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      exec,
    bool      static_lifetime)
{
    uregion_node_t* node = kmalloc(sizeof(uregion_node_t));

    uint32_t flags = 0;
    SET_FLAG(flags, F_READ, read);
    SET_FLAG(flags, F_WRITE, write);
    SET_FLAG(flags, F_EXEC, exec);
    SET_FLAG(flags, F_STATIC_LIFETIME, static_lifetime);
    SET_FLAG(flags, F_PARTIALLY_MAPPED, false);
    SET_FLAG(flags, F_FULL_MAPPED, false);
    SET_FLAG(flags, F_HAS_KNL, false);

    node->next   = NULL;
    node->region = (uregion_t) {
        .usr_start  = usr_va,
        .pages      = pages,
        .flags      = flags,
        .knl_start  = 0,
        .knl_vtoken = {0},
        .committed  = {0},
    };

    return node;
}


uregion_result_e uregion_reserve(
    task*     t,
    uintptr_t usr_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      exec)
{
    uregion_node_t* node = node_malloc(usr_va, pages, read, write, exec, false);

    bool overlaps = !region_insert(t, node);

    if (unlikely(overlaps)) {
        kfree(node);

        return UREGION_OVERLAPS;
    }

    return UREGION_OK;
}



uregion_reserve_static_result_t uregion_reserve_static(
    task*     t,
    uintptr_t usr_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      exec)
{
    uregion_node_t* node = node_malloc(usr_va, pages, read, write, exec, true);

    bool overlaps = !region_insert(t, node);

    if (unlikely(overlaps)) {
        kfree(node);

        return (uregion_reserve_static_result_t) {
            .result = UREGION_OVERLAPS,
            .knl_va = NULL,
        };
    }

    raw_kmalloc_lock();
    {
        raw_kmalloc_info rkm_info;

        node->region.knl_start = (uintptr_t)raw_kmalloc(
            pages,
            "static lifetime uregion kernel access",
            RAW_KMALLOC_DYNAMIC_CFG,
            &rkm_info);

        node->region.knl_vtoken = rkm_info.info.dynamic.vtoken;

        size_t pa_info_count = vmalloc_get_pa_count(node->region.knl_vtoken);
        vmalloc_pa_info pa_info[pa_info_count];

        vmalloc_get_pa_info(node->region.knl_vtoken, pa_info, pa_info_count);

        for (size_t i = 0; i < pa_info_count; i++) {
            size_t offset = pa_info[i].va -
                            pa_info[0].va; // pa_info comes ordered

            mmu_map_result mmu_res = mmu_map(
                &t->mapping,
                usr_va + offset,
                pa_info[i].pa,
                power_of2(pa_info[i].order) * PAGE_SIZE,
                usr_mmu_cfg_from_flags(node->region.flags),
                NULL);

            ASSERT(mmu_res == MMU_MAP_OK);
        }
    }
    raw_kmalloc_unlock();

    SET_FLAG(node->region.flags, F_HAS_KNL, true);
    SET_FLAG(node->region.flags, F_FULL_MAPPED, true);

    commited_try_init(&node->region);
    committed_set(&node->region, 0, node->region.pages);

    return (uregion_reserve_static_result_t) {
        .result = UREGION_OK,
        .knl_va = (void*)node->region.knl_start,
    };
}


void* uregion_commit(task* t, uintptr_t usrva, uint32_t pages)
{
    const char* tag = "non static lifetime uregion kernel access";

    constexpr mmu_pg_cfg KNL_ACCESS_MMU_CFG = (mmu_pg_cfg) {
        .attr_index   = 0,
        .ap           = MMU_AP_EL0_NONE_EL1_RW,
        .shareability = MMU_SH_INNER_SHAREABLE,
        .non_secure   = false,
        .access_flag  = 1,
        .pxn          = true,
        .uxn          = true,
        .sw           = 0,
    };


    DEBUG_ASSERT(usrva % PAGE_ALIGN == 0 && pages > 0);

    uregion_node_t* node = region_find_usrva(t, usrva);

    if (unlikely(node == NULL))
        return NULL;

    uregion_t* region            = &node->region;
    size_t     abs_region_offset = usrva - region->usr_start;
    size_t     abs_region_end = region->usr_start + region->pages * PAGE_SIZE;

    if (usrva + pages * PAGE_SIZE > abs_region_end)
        return NULL; // the commit gets out of range of the region


    raw_kmalloc_lock();

    if (unlikely(GET_FLAG(region->flags, F_HAS_KNL) == 0)) {
        // the kernel access vmalloc hasn't been done yet, so reserve the
        // full virtual address region
        region->knl_start = vmalloc(
            region->pages,
            tag,
            (vmalloc_cfg) {
                .kmap =
                    {
                        .use_kmap = false,
                        .kmap_pa  = 0,
                    },
                .assign_pa  = false,
                .device_mem = false,
                .permanent  = false,
            },
            &region->knl_vtoken);

        commited_try_init(region);

        SET_FLAG(region->flags, F_HAS_KNL, true);
    }

    size_t remaining = pages;
    while (remaining) {
        size_t order = log2_floor(remaining);
        size_t pgs   = power_of2(order);
        size_t bytes = pgs * PAGE_SIZE;

        // offset from provided usrva
        uintptr_t fraction_offset = (pages - remaining) * PAGE_SIZE;
        uintptr_t region_offset   = abs_region_offset + fraction_offset;


        puintptr_t pa = page_malloc(order, mm_page_data_new(tag, false, false));
        vuintptr_t knl_va = region->knl_start + region_offset;
        vuintptr_t usr_va = region->usr_start + region_offset;


        // setup kernerspace access
        vmalloc_push_pa(region->knl_vtoken, order, pa, knl_va);

        mmu_map_result mmu_res = mmu_map(
            MM_MMU_KERNEL_MAPPING,
            knl_va,
            pa,
            bytes,
            KNL_ACCESS_MMU_CFG,
            NULL);

        ASSERT(mmu_res == MMU_MAP_OK);


        // setup userspace access
        mmu_res = mmu_map(
            &t->mapping,
            usr_va,
            pa,
            bytes,
            usr_mmu_cfg_from_flags(region->flags),
            NULL);

        ASSERT(mmu_res == MMU_MAP_OK);


        // mark the commited bitfield pages
        committed_set(region, region_offset / PAGE_SIZE, pgs);

        remaining -= pgs;
    }

    raw_kmalloc_unlock();

    return (void*)(region->knl_start + abs_region_offset);
}




// uregion_free


static void region_destroy(task* t, uregion_node_t* node)
{
    mmu_unmap_result mmu_res;
    uregion_t*       region = &node->region;

    if (GET_FLAG(node->region.flags, F_FULL_MAPPED) ||
        GET_FLAG(node->region.flags, F_PARTIALLY_MAPPED)) {
        DEBUG_ASSERT(GET_FLAG(region->flags, F_HAS_KNL));

        mmu_res = mmu_unmap(
            &t->mapping,
            region->usr_start,
            region->pages * PAGE_SIZE,
            NULL);

        ASSERT(mmu_res == MMU_UNMAP_OK);

        mmu_res = mmu_unmap(
            MM_MMU_KERNEL_MAPPING,
            region->knl_start,
            region->pages * PAGE_SIZE,
            NULL);

        ASSERT(mmu_res == MMU_UNMAP_OK);


        size_t          n = vmalloc_get_pa_count(region->knl_vtoken);
        vmalloc_pa_info pa_info[n];

        vmalloc_get_pa_info(region->knl_vtoken, pa_info, n);

        for (size_t i = 0; i < n; i++) {
            page_free(pa_info[i].pa);
        }

        vfree(region->knl_vtoken, NULL);
    }


    if (region->pages > 64) {
        kfree(region->committed.bg);
    }

    region_remove(t, node);
    kfree(node);
}


bool uregion_free(task* t, uintptr_t usrva, uint32_t pages)
{
    uregion_node_t* node = region_find_usrva(t, usrva);
    if (unlikely(!node))
        return false;

    uregion_t* region = &node->region;

    uintptr_t reg_start = region->usr_start;
    uintptr_t reg_end   = region->usr_start + region->pages * PAGE_SIZE;

    uintptr_t fr_start = usrva;
    uintptr_t fr_end   = fr_start + pages * PAGE_SIZE;

    if (fr_end > reg_end)
        return false; // out of bounds

    bool trims_start = fr_start == reg_start;
    bool trims_end   = fr_end == reg_end;

    if (likely(trims_start && trims_end)) { // case 1: full region free
        region_destroy(t, node);
        return true;
    }

    PANIC("TODO: implement partial frees");
}



bool uregion_is_reserved(
    task*       t,
    uintptr_t   start,
    size_t      size,
    uregion_t** out_region)
{
    if (size == 0)
        return false;

    uregion_node_t* node = region_find_usrva(t, start);

    if (!node)
        return false;

    // check the full range fits within the found region
    uintptr_t reg_end = node->region.usr_start + node->region.pages * PAGE_SIZE;
    uintptr_t range_end = start + size;

    if (range_end > reg_end)
        return false;

    if (out_region)
        *out_region = &node->region;

    return true;
}


bool uregion_is_committed(
    task*       t,
    uintptr_t   start,
    size_t      size,
    uregion_t** out_region)
{
    uregion_t* region;

    if (!uregion_is_reserved(t, start, size, &region))
        return false;

    if (out_region)
        *out_region = region;

    // fast path fully mapped static region
    if (GET_FLAG(region->flags, F_FULL_MAPPED))
        return true;

    if (!GET_FLAG(region->flags, F_HAS_KNL) ||
        !GET_FLAG(region->flags, F_PARTIALLY_MAPPED))
        return false;

    // check bitfield for each page in [start, start+size)
    size_t first_page = (start - region->usr_start) / PAGE_SIZE;
    size_t page_count = div_ceil(size, PAGE_SIZE);

    constexpr size_t N = BITFIELD_CAPACITY(typeof(region->committed.sm));

    bitfield64* bf = region->pages > N ? region->committed.bg
                                       : &region->committed.sm;

    for (size_t i = 0; i < page_count; i++) {
        size_t idx  = first_page + i;
        size_t bf_n = idx / N;
        size_t bf_i = idx % N;

        if (!bitfield_get(bf[bf_n], bf_i))
            return false;
    }

    return true;
}


static const uintptr_t USR_ADDRSPACE_END = KERNEL_BASE &
                                           ~(0xFFFF000000000000ULL);

uintptr_t uregion_find_free(task* t, uint32_t pages)
{
    size_t size = pages * PAGE_SIZE;

    uintptr_t       prev_start = USR_ADDRSPACE_END;
    uregion_node_t* curr       = get_head_node(t); // high to low

    while (curr) {
        uintptr_t curr_end = curr->region.usr_start +
                             curr->region.pages * PAGE_SIZE;

        uintptr_t gap = prev_start - curr_end;

        if (gap >= size)
            return prev_start - size; // top of the gap, aligned down

        prev_start = curr->region.usr_start;
        curr       = curr->next;
    }

    // check gap between address 0 and the lowest region
    if (prev_start >= size)
        return align_down(prev_start - size, PAGE_SIZE);

    return UINTPTR_MAX; // no free gap found
}


bool uregion_get_knl_access(
    uregion_t* region,
    uintptr_t  usr_va,
    uintptr_t* knl_va)
{
    if (unlikely(
            usr_va < region->usr_start ||
            usr_va >= region->usr_start + region->pages * PAGE_SIZE))
        return false;

    if (!GET_FLAG(region->flags, F_HAS_KNL))
        return false;

    size_t offset = usr_va - region->usr_start;

    if (knl_va)
        *knl_va = region->knl_start + offset;

    return true;
}

bool uregion_get_flag(uregion_t* region, uregion_flags_e flag)
{
    if (!region)
        return false;

    return GET_FLAG(region->flags, flag);
}
