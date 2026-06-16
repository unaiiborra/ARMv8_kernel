#include <arm/mmu.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/mm/page_malloc.h>
#include <kernel/mm/uregion.h>
#include <kernel/mm/vmalloc.h>
#include <kernel/panic.h>
#include <kernel/task.h>
#include <lib/align.h>
#include <lib/branch.h>
#include <lib/data_structures/kvec.h>
#include <lib/data_structures/rbtree.h>
#include <lib/lock.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>


#define ASSERT_OWNER_IS_LOCKED(owner)       \
    DEBUG_ASSERT(                           \
        spinlock_is_locked(&(owner)->lock), \
        "the task should be locked before calling!")


/// flags of the entire region. It is uregion_flags_e + internal flags
typedef enum {
    F_READ             = UREGION_F_READ,
    F_WRITE            = UREGION_F_WRITE,
    F_EXEC             = UREGION_F_EXEC,
    F_STATIC_LIFETIME  = 1 << 3,
    F_FULL_MAPPED      = 1 << 4,
    F_PARTIALLY_MAPPED = 1 << 5,
    F_HAS_PHYS         = 1 << 6,
} uregion_flags_internal_e;

typedef struct {
    puintptr_t physical_address;
    vuintptr_t user_address;
    size_t     page_count;
} physdata_t;

typedef struct {
    rb_header_t __rbheader;
    physdata_t  physdata;
} physdata_node_t;

typedef struct uregion {
    uintptr_t usr_start;                    // userspace va start
    uint32_t  pages;                        // page count
    uint32_t  flags;                        // uregion_flags_internal_e
    rbtreeT(physdata_node_t) physical_data; // physical mapped pages information
    // bitfield of pages with a physical address assigned
    // if pages <= 64: sm
    // si pages >  64: bg (allocated array)
    union {
        bitfield64  sm;
        bitfield64* bg;
    } committed;
} uregion_t;

typedef struct uregion_node {
    rb_header_t _header;
    uregion_t   region;
} uregion_node_t;

typedef struct { // user memory area
    uregion_t* uregion;
    vuintptr_t user_address;
    size_t     page_count;
} uma_t;

static void committed_init(uregion_t* region)
{
    DEBUG_ASSERT((region->flags & F_HAS_PHYS) == 0);

    size_t pages = region->pages;

    bool initialized = pages <= 64 || region->committed.bg != NULL;

    if (likely(initialized))
        return;

    size_t bg_bytes      = div_ceil(pages, BITFIELD_CAPACITY(bitfield64)) *
                           sizeof(bitfield64);
    region->committed.bg = kzalloc(bg_bytes);
}

static inline void committed_set(
    uregion_t* region,
    size_t     page_idx, // relative to the region
    size_t     pages)
{
    constexpr size_t N = BITFIELD_CAPACITY(typeof(region->committed.sm));

    bitfield64* bf = region->pages > N ? region->committed.bg
                                       : &region->committed.sm;

    for (size_t i = 0; i < pages; i++) {
        size_t idx  = page_idx + i;
        size_t bf_n = idx / N;
        size_t bf_i = idx % N;
        DEBUG_ASSERT(region->pages <= N ? bf_n == 0 : true);
        DEBUG_ASSERT(
            bitfield_get(bf[bf_n], bf_i) == 0,
            "tried to commit a page that was already commited");

        bitfield_set_high(bf[bf_n], bf_i);
    }
}

typedef enum {
    COMMITED_NONE,
    COMMITED_PARTIALLY,
    COMMITED_FULLY,
} commited_state_e;

static commited_state_e get_commited_state(uregion_t* region)
{
    constexpr size_t N = BITFIELD_CAPACITY(bitfield64);

    size_t words = div_ceil(region->pages, N);

    bitfield64* bf = region->pages > N ? region->committed.bg
                                       : &region->committed.sm;

    size_t total_set = 0;

    for (size_t w = 0; w < words; w++) {
        size_t bits_in_word = (w == words - 1 && region->pages % N != 0)
                                  ? region->pages % N
                                  : N;

        uint64_t mask = bits_in_word == 64 ? UINT64_MAX
                                           : (UINT64_C(1) << bits_in_word) - 1;

        total_set += __builtin_popcountll(bf[w] & mask);
    }

    if (total_set == 0)
        return COMMITED_NONE;

    if (total_set == region->pages)
        return COMMITED_FULLY;

    return COMMITED_PARTIALLY;
}

static inline mmu_pg_cfg usr_mmu_cfg_from_flags(uint32_t flags)
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
        case F_READ | F_WRITE:
            return RW;

        case F_READ:
            return R;

        case F_READ | F_EXEC:
            return RX;

        default:
            PANIC("mmu_cfg_from_flags: invalid flags");
    }
}

static rbt_condition_e find_region_condition(void* usrva_ctx, void* node_b)
{
    uintptr_t       usrva = (uintptr_t)usrva_ctx;
    uregion_node_t* node  = node_b;
    if (usrva < node->region.usr_start)
        return RBT_LESS_THAN;
    if (usrva >= node->region.usr_start + node->region.pages * PAGE_SIZE)
        return RBT_GREATER_THAN;
    return RBT_EQUALS;
}

static rbt_condition_e insert_region_condition(void* node_a, void* node_b)
{
    const uregion_node_t *a = node_a, *b = node_b;
    uintptr_t             a_start = a->region.usr_start;
    uintptr_t             b_start = b->region.usr_start;
    uintptr_t a_end = a_start + ((uintptr_t)a->region.pages * PAGE_SIZE);
    uintptr_t b_end = b_start + ((uintptr_t)b->region.pages * PAGE_SIZE);
    if (a_end <= b_start)
        return RBT_LESS_THAN;
    if (a_start >= b_end)
        return RBT_GREATER_THAN;
    return RBT_EQUALS;
}

// finds the node whose region contains the usr va
static uregion_node_t* region_find_usrva(const task_t* t, uintptr_t usrva)
{
    uregion_node_t*   node;
    rbt_find_result_e fres = rbt_find(
        &t->regions,
        find_region_condition,
        (void*)usrva,
        (void**)&node);

    if (likely(fres == RBT_FIND_OK))
        return node;

    return NULL;
}

// ordered by usr_start, returns false if it overlaps (and does not insert)
static bool region_insert(task_t* t, uregion_node_t* node)
{
    return rbt_insert(&t->regions, node, insert_region_condition) ==
           RBT_INSERT_OK;
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

    if (read)
        flags |= F_READ;

    if (write)
        flags |= F_WRITE;

    if (exec)
        flags |= F_EXEC;

    if (static_lifetime)
        flags |= F_STATIC_LIFETIME;

    node->region = (uregion_t) {
        .usr_start     = usr_va,
        .pages         = pages,
        .flags         = flags,
        .physical_data = RBT_INIT,
        .committed     = {0},
    };

    return node;
}

typedef enum : int32_t {
    PDATA_CONDITION_LEFT          = RBT_LESS_THAN,
    PDATA_CONDITION_RIGHT         = RBT_GREATER_THAN,
    PDATA_CONDITION_START_MATCHES = RBT_EQUALS,
    // custom results
    PDATA_CONDITION_OVERLAPS,
} pdata_tree_res_e;

typedef enum : int32_t {
    PDATA_INSERT_OK = RBT_INSERT_OK,
    // __PDATA_INSERT_RESERVED = RBT_INSERT_RESERVED,
    PDATA_INSERT_EXISTS = RBT_INSERT_EXISTS,
    // custom results
    PDATA_INSERT_OVERLAPS = PDATA_CONDITION_OVERLAPS,
} pdata_insert_res_e;

static rbt_condition_e pdata_insert_condition(void* node_a, void* node_b)
{
    physdata_t *a = &((physdata_node_t*)node_a)->physdata,
               *b = &((physdata_node_t*)node_b)->physdata;

    if (a->user_address < b->user_address) {
        vuintptr_t a_end = a->user_address + a->page_count * PAGE_SIZE;

        if (unlikely(a_end > b->user_address))
            return (rbt_condition_e)PDATA_CONDITION_OVERLAPS;

        return (rbt_condition_e)PDATA_CONDITION_LEFT;
    }

    if (a->user_address > b->user_address) {
        vuintptr_t b_end = b->user_address + b->page_count * PAGE_SIZE;

        if (unlikely(b_end > a->user_address))
            return (rbt_condition_e)PDATA_CONDITION_OVERLAPS;

        return (rbt_condition_e)PDATA_CONDITION_RIGHT;
    }

    return (rbt_condition_e)PDATA_CONDITION_START_MATCHES;
}

static rbt_condition_e pdata_find_condition(void* cmp_ctx, void* tree_node)
{
    uintptr_t   target = (uintptr_t)cmp_ctx;
    physdata_t* phys   = &((physdata_node_t*)tree_node)->physdata;

    uintptr_t start = phys->user_address;
    uintptr_t end   = start + phys->page_count * PAGE_SIZE;

    if (target < start)
        return RBT_LESS_THAN;

    if (target >= end)
        return RBT_GREATER_THAN;

    return RBT_EQUALS;
}

static pdata_insert_res_e region_insert_user_physdata(
    uregion_t* uregion,
    vuintptr_t usr_va,
    puintptr_t physical_address,
    size_t     page_count)
{
    DEBUG_ASSERT(
        uregion->usr_start <= usr_va &&
        uregion->usr_start + uregion->pages * PAGE_SIZE >=
            usr_va + page_count * PAGE_SIZE);

    physdata_node_t* node = kmalloc(sizeof(physdata_node_t));

    node->physdata = (physdata_t) {
        .physical_address = physical_address,
        .user_address     = usr_va,
        .page_count       = page_count,
    };

    int32_t ires = rbt_insert(
        &uregion->physical_data,
        node,
        pdata_insert_condition);

    DEBUG_ASSERT(
        ires == PDATA_INSERT_OK || ires == PDATA_CONDITION_OVERLAPS ||
        ires == PDATA_INSERT_EXISTS);

    return ires;
}

static void map_user_region(
    mmu_mapping* mapping,
    uregion_t*   uregion,
    uintptr_t    usr_va,
    size_t       pages,
    uint32_t     flags,
    const char*  tag,
    bool         zeroed)
{
    size_t rem = pages;

    while (rem > 0) {
        size_t order      = log2_floor(rem);
        size_t page_count = power_of2(order);

        // map kernel access as kmap
        raw_kmalloc_info rkm_info;
        raw_kmalloc(
            page_count,
            tag,
            &(raw_kmalloc_cfg) {
                .assign_pa    = true,
                .fill_reserve = true,
                .device_mem   = false,
                .permanent    = false,
                .kmap         = true,
                .init_zeroed  = zeroed,
            },
            &rkm_info);

        puintptr_t physical_address = rkm_info.info.kmap.pv.pa;
        uintptr_t  user_address     = usr_va + ((pages - rem) * PAGE_SIZE);

        pdata_insert_res_e ires = region_insert_user_physdata(
            uregion,
            user_address,
            physical_address,
            page_count);

        ASSERT(ires == PDATA_INSERT_OK);

        // map contiguous user access
        mmu_map_result mmu_res = mmu_map(
            mapping,
            user_address,
            physical_address,
            page_count * PAGE_SIZE,
            usr_mmu_cfg_from_flags(flags),
            NULL);

        ASSERT(mmu_res == MMU_MAP_OK);

        rem -= page_count;
    }
}

static void commit(
    uregion_t*   uregion,
    mmu_mapping* mapping,
    uintptr_t    usrva,
    uint32_t     pages,
    bool         zeroed)
{
    DEBUG_ASSERT(
        (uregion->flags & F_FULL_MAPPED) == 0,
        "uregion_commit called on already fully committed region");

    size_t abs_region_end = uregion->usr_start + uregion->pages * PAGE_SIZE;

    if (unlikely(usrva + pages * PAGE_SIZE > abs_region_end))
        PANIC("the commit gets out of range of the region");

    if (unlikely((uregion->flags & F_HAS_PHYS) == 0)) {
        committed_init(uregion); // init commited field in uregion
        uregion->flags |= F_HAS_PHYS;
    }

    map_user_region(
        mapping,
        uregion,
        usrva,
        pages,
        uregion->flags,
        "commited user page",
        zeroed);

    committed_set(uregion, (usrva - uregion->usr_start) / PAGE_SIZE, pages);

    switch (get_commited_state(uregion)) {
        case COMMITED_PARTIALLY: {
            uregion->flags |= F_PARTIALLY_MAPPED;
            uregion->flags &= ~F_FULL_MAPPED;
        } break;

        case COMMITED_FULLY: {
            uregion->flags |= F_FULL_MAPPED;
            uregion->flags &= ~F_PARTIALLY_MAPPED;
        } break;

        case COMMITED_NONE: {
            PANIC("committed none after uregion_commit");
        } break;
    }
}

uregion_reserve_e uregion_reserve(
    task_t*   t,
    uintptr_t usr_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      exec)
{
    ASSERT_OWNER_IS_LOCKED(t);

    uregion_node_t* node = node_malloc(usr_va, pages, read, write, exec, false);

    bool overlaps = !region_insert(t, node);

    if (unlikely(overlaps)) {
        kfree(node);

        return UREGION_OVERLAPS;
    }

    return UREGION_OK;
}

uregion_reserve_e uregion_reserve_static(
    task_t*   t,
    uintptr_t usr_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      exec,
    bool      zeroed)
{
    ASSERT_OWNER_IS_LOCKED(t);

    uregion_node_t* node = node_malloc(usr_va, pages, read, write, exec, true);

    bool overlaps = !region_insert(t, node);

    if (unlikely(overlaps)) {
        kfree(node);

        return UREGION_OVERLAPS;
    }

    map_user_region(
        &t->mapping,
        &node->region,
        usr_va,
        pages,
        node->region.flags,
        "static uregion kernel access",
        zeroed);

    committed_init(&node->region);
    committed_set(&node->region, 0, node->region.pages);

    node->region.flags |= F_HAS_PHYS | F_FULL_MAPPED;

    return UREGION_OK;
}

void uregion_commit(task_t* t, uintptr_t usrva, uint32_t pages, bool zeroed)
{
    ASSERT_OWNER_IS_LOCKED(t);

    uregion_node_t* node = region_find_usrva(t, usrva);

    if (unlikely(node == NULL))
        PANIC("attempted to commit into a unreserved uregion");

    DEBUG_ASSERT(usrva % PAGE_ALIGN == 0 && pages > 0);
    DEBUG_ASSERT(
        (node->region.flags & F_FULL_MAPPED) == 0,
        "uregion_commit called on already fully committed region");

    uregion_t* uregion        = &node->region;
    size_t     abs_region_end = uregion->usr_start + uregion->pages * PAGE_SIZE;

    if (usrva + pages * PAGE_SIZE > abs_region_end)
        PANIC("the commit gets out of range of the region");

    commit(uregion, &t->mapping, usrva, pages, zeroed);
}

static void physdata_destroy(void* physdata_node, void* ctx)
{
    mmu_mapping* mapping  = ctx;
    physdata_t   physdata = ((physdata_node_t*)physdata_node)->physdata;

    // unmaps kernel direct access and frees the physical pages
    raw_kfree(/* get the direct access address */
              kpa_to_kva_pt(physdata.physical_address));

    // unmap user access
    mmu_unmap_result ures = mmu_unmap(
        mapping,
        physdata.user_address,
        physdata.page_count * PAGE_SIZE,
        NULL);

    ASSERT(ures == MMU_UNMAP_OK);

    // free the node
    kfree(physdata_node);
}

static void region_destroy(task_t* t, uregion_node_t* node)
{
    uregion_t* region = &node->region;

    // iters over all the nodes and calls physdata_destroy
    rbt_destroy(&region->physical_data, physdata_destroy, &t->mapping);

    if (region->pages > 64)
        kfree(region->committed.bg);

    kfree(rbt_remove(&t->regions, node));
}

bool uregion_free(task_t* t, uintptr_t usrva, uint32_t pages)
{
    ASSERT_OWNER_IS_LOCKED(t);

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
    const task_t* t,
    uintptr_t     start,
    size_t        size,
    uregion_t**   out_region)
{
    ASSERT_OWNER_IS_LOCKED(t);

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
    const task_t* t,
    uintptr_t     start,
    size_t        size,
    uregion_t**   out_region)
{
    ASSERT_OWNER_IS_LOCKED(t);

    uregion_t* region;

    if (!uregion_is_reserved(t, start, size, &region))
        return false;

    if (out_region)
        *out_region = region;

    // fast path fully mapped static region
    if ((region->flags & F_FULL_MAPPED) != 0)
        return true;

    if ((region->flags & F_HAS_PHYS) == 0)
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

static inline uregion_access_e unode_check_access(
    const task_t*   t,
    uregion_node_t* unode,
    uintptr_t       start,
    uintptr_t       end,
    uregion_flags_e required_flags,
    uregion_flags_e forbidden_flags,
    bool            commit_on_demand,
    kvec(uma_t) * to_commit,
    uintptr_t* chunk_end_out)
{
    if (!unode)
        return UREGION_ACCESS_NOT_RESERVED;

    uregion_t* uregion = &unode->region;

    if ((uregion->flags & required_flags) != required_flags ||
        (uregion->flags & forbidden_flags) != 0)
        return UREGION_ACCESS_NO_PERMISSION;

    uintptr_t region_end = uregion->usr_start + uregion->pages * PAGE_SIZE;
    uintptr_t chunk_end  = min(end, region_end);
    size_t    chunk_size = chunk_end - start;

    DEBUG_ASSERT(chunk_end > start);
    DEBUG_ASSERT(chunk_size % PAGE_SIZE == 0);

    if (!uregion_is_committed(t, start, chunk_size, NULL)) {
        if (!commit_on_demand)
            return UREGION_ACCESS_NOT_COMMITTED;

        // commit on demand
        if (to_commit != NULL)
            kvec_push(
                to_commit,
                &(uma_t) {
                    .uregion      = uregion,
                    .user_address = start,
                    .page_count   = chunk_size / PAGE_SIZE,
                });
    }

    if (chunk_end_out)
        *chunk_end_out = chunk_end;

    return UREGION_ACCESS_OK;
}

uregion_access_e uregions_check_access(
    task_t*         t,
    uintptr_t       start,
    size_t          size,
    uregion_flags_e required_flags,
    uregion_flags_e forbidden_flags,
    bool            commit_on_demand)
{
    ASSERT_OWNER_IS_LOCKED(t);

    scoped_kvec(uma_t) to_commit = kvec_new(uma_t);
    uintptr_t end                = align_up(start + size, PAGE_ALIGN);
    start                        = align_down(start, PAGE_ALIGN);
    uintptr_t cursor             = start;

    // first pass, check access and collect areas to commit
    while (cursor < end) {
        uregion_node_t* unode = region_find_usrva(t, cursor);

        uintptr_t        chunk_end;
        uregion_access_e uaccess = unode_check_access(
            t,
            unode,
            cursor,
            end,
            required_flags,
            forbidden_flags,
            commit_on_demand,
            &to_commit,
            &chunk_end);

        if (uaccess != UREGION_ACCESS_OK)
            return uaccess;

        cursor = chunk_end;
    }

    // second pass, commit collected areas
    uma_t* uma = kvec_dataT(uma_t, &to_commit);

    for (size_t i = 0; i < kvec_len(&to_commit); i++)
        commit(
            uma[i].uregion,
            &t->mapping,
            uma[i].user_address,
            uma[i].page_count,
            true);

    return UREGION_ACCESS_OK;
}

static constexpr uintptr_t USR_ADDRSPACE_END = KERNEL_BASE &
                                               ~(0xFFFF000000000000ULL);

typedef struct {
    size_t     size;
    uintptr_t  prev_start;
    uintptr_t* out;
    bool       found;
} find_free_ctx_t;

static bool find_free_visitor(void* node, void* ctx)
{
    uregion_node_t*  curr = node;
    find_free_ctx_t* fctx = ctx;

    uintptr_t curr_end = curr->region.usr_start +
                         curr->region.pages * PAGE_SIZE;

    uintptr_t gap = fctx->prev_start - curr_end;

    if (gap >= fctx->size) {
        if (fctx->out)
            *fctx->out = align_down(fctx->prev_start - fctx->size, PAGE_SIZE);

        fctx->found = true;
        return false; // stop
    }

    fctx->prev_start = curr->region.usr_start;
    return true;
}

bool uregion_find_free(task_t* t, uint32_t pages, uintptr_t* out)
{
    ASSERT_OWNER_IS_LOCKED(t);

    if (!pages)
        return false;

    size_t size = (size_t)pages * PAGE_SIZE;

    find_free_ctx_t ctx = {
        .size       = size,
        .prev_start = USR_ADDRSPACE_END,
        .out        = out,
        .found      = false,
    };

    rbt_for_each_rev(&t->regions, find_free_visitor, &ctx);

    if (!ctx.found && ctx.prev_start >= size) {
        if (out)
            *out = align_down(ctx.prev_start - size, PAGE_SIZE);

        return true;
    }

    return ctx.found;
}

uint32_t uregion_get_flags(uregion_t* region)
{
    if (!region)
        return false;

    return region->flags & (F_READ | F_WRITE | F_EXEC);
}

uregion_access_e umemcpy(
    task_t*         t,
    void*           dst,
    const void*     src,
    size_t          size,
    uregion_flags_e required_flags,
    uregion_flags_e forbidden_flags,
    bool            commit_on_demand,
    umemcpy_type_e  type)
{
    DEBUG_ASSERT(type == UMEMCPY_USR_TO_KNL || type == UMEMCPY_KNL_TO_USR);

    physdata_node_t* pnode;
    size_t           off       = 0;
    uintptr_t        usr_start = (type == UMEMCPY_KNL_TO_USR) ? (uintptr_t)dst
                                                              : (uintptr_t)src;

    // first pass, check that the memcpy can be done
    uregion_access_e access = uregions_check_access(
        t,
        usr_start,
        size,
        required_flags,
        forbidden_flags,
        commit_on_demand);

    if (access != UREGION_ACCESS_OK)
        return access;

    // second pass, perform the memcpy
    while (off < size) {
        uintptr_t       usr_ptr = usr_start + off;
        uregion_node_t* unode   = region_find_usrva(t, usr_start + off);

        rbt_find_result_e fres = rbt_find(
            &unode->region.physical_data,
            pdata_find_condition,
            (void*)usr_ptr,
            (void**)&pnode);

        DEBUG_ASSERT(fres == RBT_FIND_OK && pnode);

        size_t node_offset  = usr_ptr - pnode->physdata.user_address;
        size_t block_remain = pnode->physdata.page_count * PAGE_SIZE -
                              node_offset;
        size_t chunk        = min(block_remain, size - off);

        void* knl_ptr = kpa_to_kva_pt(
            pnode->physdata.physical_address + node_offset);

        if (type == UMEMCPY_KNL_TO_USR)
            memcpy(knl_ptr, (uint8_t*)src + off, chunk);
        else
            memcpy((uint8_t*)dst + off, knl_ptr, chunk);

        off += chunk;
    }

    return UREGION_ACCESS_OK;
}

uregion_access_e ustrncpy(
    const task_t*   t,
    char*           kernel_buf,
    char*           usr_string,
    uregion_flags_e required_flags,
    uregion_flags_e forbidden_flags,
    size_t          max)
{
    ASSERT_OWNER_IS_LOCKED((task_t*)t);

    size_t    off       = 0;
    uintptr_t usr_start = (uintptr_t)usr_string;

    while (off < max) {
        uintptr_t       usr_ptr = usr_start + off;
        uregion_node_t* unode   = region_find_usrva(t, usr_start + off);

        uintptr_t        chunk_end;
        uregion_access_e uaccess = unode_check_access(
            t,
            unode,
            usr_start + off,
            usr_start + max,
            required_flags,
            forbidden_flags,
            false,
            NULL,
            &chunk_end);

        if (uaccess != UREGION_ACCESS_OK)
            return uaccess;

        physdata_node_t*  pnode = NULL;
        rbt_find_result_e fres  = rbt_find(
            &unode->region.physical_data,
            pdata_find_condition,
            (void*)usr_ptr,
            (void**)&pnode);

        DEBUG_ASSERT(fres == RBT_FIND_OK && pnode);

        size_t node_offset  = usr_ptr - pnode->physdata.user_address;
        size_t block_remain = pnode->physdata.page_count * PAGE_SIZE -
                              node_offset;
        size_t chunk        = min(block_remain, max - off);

        const char* knl_ptr = kpa_to_kva_pt(
            pnode->physdata.physical_address + node_offset);

        for (size_t i = 0; i < chunk; i++, off++) {
            kernel_buf[off] = knl_ptr[i];

            if (knl_ptr[i] == '\0')
                return UREGION_ACCESS_OK;
        }
    }

    kernel_buf[max - 1] = '\0';
    return UREGION_ACCESS_TRUNCATED;
}

uregion_access_e umemzero(
    task_t*         t,
    void*           usr_dst,
    size_t          size,
    uregion_flags_e required_flags,
    uregion_flags_e forbidden_flags,
    bool            commit_on_demand)
{
    physdata_node_t* pnode;
    size_t           off       = 0;
    uintptr_t        usr_start = (uintptr_t)usr_dst;

    // first pass, check that the memzero can be done
    uregion_access_e access = uregions_check_access(
        t,
        usr_start,
        size,
        required_flags,
        forbidden_flags,
        commit_on_demand);

    if (access != UREGION_ACCESS_OK)
        return access;

    while (off < size) {
        uintptr_t       usr_ptr = usr_start + off;
        uregion_node_t* unode   = region_find_usrva(t, usr_start + off);

        rbt_find_result_e fres = rbt_find(
            &unode->region.physical_data,
            pdata_find_condition,
            (void*)usr_ptr,
            (void**)&pnode);

        DEBUG_ASSERT(fres == RBT_FIND_OK && pnode);

        size_t node_offset  = usr_ptr - pnode->physdata.user_address;
        size_t block_remain = pnode->physdata.page_count * PAGE_SIZE -
                              node_offset;
        size_t chunk        = min(block_remain, size - off);

        void* knl_ptr = kpa_to_kva_pt(
            pnode->physdata.physical_address + node_offset);

        memzero(knl_ptr, chunk);

        off += chunk;
    }

    return UREGION_ACCESS_OK;
}

void uregion_flush_icache(task_t* t, uintptr_t usr_start, size_t size)
{
    ASSERT_OWNER_IS_LOCKED(t);

    uintptr_t end    = align_up(usr_start + size, PAGE_ALIGN);
    uintptr_t cursor = align_down(usr_start, PAGE_ALIGN);
    size_t    off    = 0;

    while (cursor < end) {
        uregion_node_t* unode = region_find_usrva(t, cursor);
        DEBUG_ASSERT(unode);

        physdata_node_t* pnode = NULL;
        rbt_find(
            &unode->region.physical_data,
            pdata_find_condition,
            (void*)cursor,
            (void**)&pnode);
        DEBUG_ASSERT(pnode);

        size_t node_offset  = cursor - pnode->physdata.user_address;
        size_t block_remain = pnode->physdata.page_count * PAGE_SIZE -
                              node_offset;
        size_t chunk        = min(block_remain, size - off);

        void* knl_ptr = kpa_to_kva_pt(
            pnode->physdata.physical_address + node_offset);

        cache_flush_range(knl_ptr, (void*)((uintptr_t)knl_ptr + chunk));

        cursor += chunk;
        off += chunk;
    }
}
