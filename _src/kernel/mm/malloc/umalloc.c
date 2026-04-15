#include "lib/lock.h"
#define UMALLOC_IMPLEMENTATION

#include <arm/mmu.h>
#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/mm/page_malloc.h>
#include <kernel/mm/umalloc.h>
#include <kernel/mm/vmalloc.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <kernel/task.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>

#define BIT(bit) (1U << bit)
#define SET_FLAG(flags, bit, val) \
    ((flags) = ((flags) & ~(1U << (bit))) | ((unsigned int)(!!(val)) << (bit)))

#define GET_FLAG(flags, bit) (bool)(((flags) >> (bit)) & 1U)


typedef struct region_node {
    struct region_node* next;
    task_region         reg;
} region_node;


static const mmu_pg_cfg KNL_MMU_CFG = (mmu_pg_cfg) {
    .attr_index   = 0,
    .ap           = MMU_AP_EL0_NONE_EL1_RW,
    .shareability = MMU_SH_INNER_SHAREABLE,
    .non_secure   = false,
    .access_flag  = 1,
    .pxn          = true,
    .uxn          = true,
    .sw           = 0,
};


static inline region_node* node_from_region(task_region* r)
{
    if (!r)
        return NULL;

    return (region_node*)((char*)r - offsetof(region_node, reg));
}


static inline mmu_pg_cfg usr_mmu_cfg_from_flags(uint32_t flags)
{
    switch (flags & 0b111U) {
        case BIT(UMALLOC_F_READ) | BIT(UMALLOC_F_WRITE):
            return mmu_pg_cfg_new(
                0,
                MMU_AP_EL0_RW_EL1_RW,
                MMU_SH_INNER_SHAREABLE,
                false,
                true,
                true,
                true,
                0);

        case BIT(UMALLOC_F_READ):
            return mmu_pg_cfg_new(
                0,
                MMU_AP_EL0_RO_EL1_RO,
                MMU_SH_INNER_SHAREABLE,
                false,
                true,
                true,
                true,
                0);

        case BIT(UMALLOC_F_READ) | BIT(UMALLOC_F_EXEC):
            return mmu_pg_cfg_new(
                0,
                MMU_AP_EL0_RO_EL1_RO,
                MMU_SH_INNER_SHAREABLE,
                false,
                true,
                true,
                false,
                0);

        default:
            PANIC("mmu_cfg_from_flags: invalid flags");
    }
}


/// pushes the new region ordered by usr address
static void push_usr_region_to_task(task* utask, task_region region)
{
    uintptr_t new_start = region.reg_start;

    region_node* prev = NULL;
    region_node* cur  = node_from_region(utask->regions);

    while (cur && cur->reg.reg_start < new_start) {
        prev = cur;
        cur  = cur->next;
    }

#ifdef DEBUG
    uintptr_t new_end = region.reg_start + region.pages * PAGE_SIZE;

    // check overlap with prev
    if (prev) {
        uintptr_t prev_end = prev->reg.reg_start + prev->reg.pages * PAGE_SIZE;

        if (prev_end > new_start)
            PANIC("user regions overlaps");
    }

    // check overlap with next
    if (cur) {
        uintptr_t cur_start = cur->reg.reg_start;

        if (new_end > cur_start)
            PANIC("user regions overlaps");
    }
#endif

    region_node* node = kmalloc(sizeof(region_node));
    node->reg         = region;
    node->next        = cur;

    if (prev)
        prev->next = node;
    else
        utask->regions = &node->reg;
}


static void umalloc_subregion(
    mmu_mapping* mapping,
    task_region* region,
    const char*  tag,
    uintptr_t    subregion_start,
    uint32_t     pages)
{
    DEBUG_ASSERT(region->knl_start != 0);
    DEBUG_ASSERT(
        region->reg_start <= subregion_start &&
            region->reg_start + region->pages * PAGE_SIZE >=
                subregion_start + pages * PAGE_SIZE,
        "umalloc_subregion: subregion out of region");


    uintptr_t knl_subregion_start =
        region->knl_start + (subregion_start - region->reg_start);

    uint32_t remaining_pages = pages;

    while (remaining_pages) {
        uintptr_t offset = (pages - remaining_pages) * PAGE_SIZE;
        size_t    order  = log2_floor(remaining_pages);

        puintptr_t pa = page_malloc(order, mm_page_data_new(tag, false, false));

        mmu_map_result mres;
        // map kernel access
        mres = mmu_map(
            MM_MMU_KERNEL_MAPPING,
            knl_subregion_start + offset,
            pa,
            power_of2(order) * PAGE_SIZE,
            KNL_MMU_CFG,
            NULL);
        ASSERT(mres == MMU_MAP_OK);

        // map user access
        mres = mmu_map(
            mapping,
            subregion_start + offset,
            pa,
            power_of2(order) * PAGE_SIZE,
            usr_mmu_cfg_from_flags(region->flags),
            NULL);
        ASSERT(mres == MMU_MAP_OK);

        remaining_pages -= power_of2(order);
    }
}


void* umalloc(
    task*     t,
    uintptr_t reg_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      execute,
    bool      static_lifetime)
{
    ASSERT(pages > 0);
    DEBUG_ASSERT(
        !spin_try_lock(&t->lock),
        "task should be locked before calling umalloc");


    // if pages > 64 the assigned pa bitfield is saved as an allocated array of
    // bitfielf64, else it is just one bitfield64
    task_region ur = (task_region) {
        .pages       = pages,
        .flags       = 0,
        .reg_start   = reg_va,
        .knl_start   = 0,
        .assigned_pa = {0} // NULL or 0 bitfield64
    };


    SET_FLAG(ur.flags, UMALLOC_F_READ, read);
    SET_FLAG(ur.flags, UMALLOC_F_WRITE, write);
    SET_FLAG(ur.flags, UMALLOC_F_EXEC, execute);
    SET_FLAG(ur.flags, UMALLOC_F_STATIC_LIFETIME, static_lifetime);
    SET_FLAG(ur.flags, UMALLOC_F_FULL_MAPPED, static_lifetime);
    SET_FLAG(ur.flags, UMALLOC_F_PARTIALLY_MAPPED, false);


    if (!static_lifetime) {
        push_usr_region_to_task(t, ur);
        return NULL;
    }


    raw_kmalloc_info kinfo;


    bitfield64* assigned_pa;

    if (pages > 64) {
        // big
        ur.assigned_pa.bg = kmalloc(
            DIV_CEIL(pages, BITFIELD_CAPACITY(bitfield64)) *
            sizeof(bitfield64));

        assigned_pa = ur.assigned_pa.bg;
    }
    else // small
        assigned_pa = &ur.assigned_pa.sm;


    // allocate the kernel access and assign the pa
    ur.knl_start = (vuintptr_t)
        raw_kmalloc(pages, t->name, &RAW_KMALLOC_DYNAMIC_CFG, &kinfo);


    size_t          n = vmalloc_get_pa_count(kinfo.info.dynamic.vtoken);
    vmalloc_pa_info pa_info[n];

    bool res = vmalloc_get_pa_info(kinfo.info.dynamic.vtoken, pa_info, n);
    ASSERT(res);

    // map each allocated phys addresses from the kernel access to the user va
    for (size_t i = 0; i < n; i++) {
        // pa_info comes ordered by va
        size_t offset = pa_info[i].va - pa_info[0].va;

        mmu_map_result mres = mmu_map(
            &t->mapping,
            reg_va + offset,
            pa_info[i].pa,
            power_of2(pa_info[i].order) * PAGE_SIZE,
            usr_mmu_cfg_from_flags(ur.flags),
            NULL);

        ASSERT(mres == MMU_MAP_OK);


        // mark the pages as allocated
        size_t count = power_of2(pa_info[i].order);
        size_t idx   = offset / PAGE_SIZE;

        for (size_t j = 0; j < count; j++) {
            size_t bf_n = (idx + j) / 64;
            size_t bf_i = (idx + j) % 64;
#ifdef DEBUG
            if (ur.pages <= 64)
                DEBUG_ASSERT(bf_n == 0);
#endif
            bitfield_set_high(assigned_pa[bf_n], bf_i);
        }
    }

    push_usr_region_to_task(t, ur);

    return (void*)ur.knl_start;
}


void* umalloc_assign_pa(task* t, uintptr_t usr_va, uint32_t pages)
{
    // find the usr va node
    region_node* cur = node_from_region(t->regions);

    ASSERT(cur, "umalloc_assign_pa: task has no allocated regions");

    uintptr_t start, end, cur_start, cur_end;
    start = usr_va;
    end   = usr_va + pages * PAGE_SIZE;

    while (cur) {
        cur_start = cur->reg.reg_start;
        cur_end   = cur->reg.reg_start + cur->reg.pages * PAGE_SIZE;

        if (cur_start <= start && cur_end >= end)
            break;

        cur = cur->next;
    }

    ASSERT(
        cur,
        "umalloc_assign: no allocated region matches with the requested "
        "subregion");
    ASSERT(!GET_FLAG(cur->reg.flags, UMALLOC_F_FULL_MAPPED));


    // if the region has no subregions assigned, it does not have a kernel
    // access to the region allocated with vmalloc
    if (cur->reg.knl_start == 0) {
        // reserve the kernel virtual region (all the region, not only the
        // subregion)
        vmalloc_token vtoken;
        cur->reg.knl_start = vmalloc(
            cur->reg.pages,
            "usr task region kernel access",
            (vmalloc_cfg) {
                .kmap =
                    {
                        .use_kmap = false,
                        .kmap_pa  = 0,
                    },
                .assing_pa  = false,
                .device_mem = false,
                .permanent  = false,
            },
            &vtoken);
    }

    umalloc_subregion(&t->mapping, &cur->reg, t->name, usr_va, pages);

    size_t offset = usr_va - cur->reg.reg_start;

    return (void*)(cur->reg.knl_start + offset);
}


void ufree(task* t, uintptr_t usr_va)
{
    region_node* cur  = node_from_region(t->regions);
    region_node* prev = NULL;

    while (cur) {
        if (cur->reg.reg_start == usr_va) {
            size_t pages = cur->reg.pages;

            if (prev)
                prev->next = cur->next;
            else
                t->regions = &cur->next->reg;

            mmu_unmap_result ures =
                mmu_unmap(&t->mapping, usr_va, pages * PAGE_SIZE, NULL);
            ASSERT(ures);

            // free the allocated bitfield64 array if it is a big region
            if (cur->reg.pages > 64 && cur->reg.assigned_pa.bg != NULL)
                kfree(cur->reg.assigned_pa.bg);

            kfree(cur);

            return;
        }

        prev = cur;
        cur  = cur->next;
    }

    PANIC("ufree: region not found");
}


bool uregion_is_mapped(
    task*         t,
    uintptr_t     start,
    size_t        size,
    task_region** out_region)
{
    if (size == 0)
        return false;

    uintptr_t end = start + size;

    region_node* cur = node_from_region(t->regions);

    while (cur) {
        uintptr_t r_start = cur->reg.reg_start;
        uintptr_t r_end   = r_start + cur->reg.pages * PAGE_SIZE;

        if (r_start <= start && r_end >= end) {
            if (out_region)
                *out_region = &cur->reg;

            return true;
        }

        cur = cur->next;
    }

    return false;
}


bool uregion_is_assigned(
    task*         t,
    uintptr_t     start,
    size_t        size,
    task_region** out_region)
{
    task_region* r;

    if (!uregion_is_mapped(t, start, size, &r))
        return false;

    if (out_region)
        *out_region = r;

    if (GET_FLAG(r->flags, UMALLOC_F_FULL_MAPPED))
        return true;

    if (r->knl_start == 0)
        return false;

    uintptr_t r_start = r->reg_start;

    size_t first_page = (start - r_start) / PAGE_SIZE;
    size_t pages      = DIV_CEIL(size, PAGE_SIZE);

    bitfield64* assigned_pa;

    if (r->pages > 64)
        assigned_pa = r->assigned_pa.bg;
    else
        assigned_pa = &r->assigned_pa.sm;

    for (size_t i = 0; i < pages; i++) {
        size_t idx  = first_page + i;
        size_t bf_n = idx / 64;
        size_t bf_i = idx % 64;

        if (!bitfield_get(assigned_pa[bf_n], bf_i))
            return false;
    }

    return true;
}


static const uintptr_t LO_ADDRSPACE_START = 0x0;
static const uintptr_t LO_ADDRSPACE_END   = KERNEL_BASE & ~(0xFFFF000000000000);

uintptr_t tregion_find_free(task* t, size_t pages)
{
    size_t size = pages * PAGE_SIZE;

    uintptr_t prev_end = LO_ADDRSPACE_START;
    uintptr_t best     = UINTPTR_MAX;

    region_node* cur = node_from_region(t->regions);

    while (cur) {
        uintptr_t start = cur->reg.reg_start;

        uintptr_t gap = start - prev_end;

        if (gap >= size) {
            uintptr_t candidate = align_down(start - size, PAGE_SIZE);
            best                = candidate;
        }

        prev_end = start + cur->reg.pages * PAGE_SIZE;
        cur      = cur->next;
    }

    uintptr_t gap = LO_ADDRSPACE_END - prev_end;

    if (gap >= size)
        best = align_down(LO_ADDRSPACE_END - size, PAGE_SIZE);

    return best;
}
