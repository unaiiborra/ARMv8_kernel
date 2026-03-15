#include <kernel/mm/mmu.h>
#include <kernel/mm/umalloc.h>
#include <kernel/scheduler.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/mmu.h"
#include "kernel/mm.h"
#include "kernel/mm/page_malloc.h"
#include "kernel/mm/vmalloc.h"
#include "kernel/panic.h"
#include "lib/math.h"
#include "lib/mem.h"
#include "lib/stdbitfield.h"


#define F_READ 0
#define F_WRITE 1
#define F_EXEC 2
#define F_STATIC_LIFETIME 3
#define F_FULL_MAPPED 4
#define F_PARTIALLY_MAPPED 5


#define BIT(bit) (1U << bit)

#define SET_FLAG(flags, bit, val) \
    ((flags) = ((flags) & ~(1U << (bit))) | ((unsigned int)(!!(val)) << (bit)))

#define GET_FLAG(flags, bit) (bool)(((flags) >> (bit)) & 1U)


static const mmu_pg_cfg KNL_MMU_CFG = (mmu_pg_cfg) {
    .attr_index = 0,
    .ap = MMU_AP_EL0_NONE_EL1_RW,
    .shareability = MMU_SH_INNER_SHAREABLE,
    .non_secure = false,
    .access_flag = 1,
    .pxn = true,
    .uxn = true,
    .sw = 0,
};

static inline mmu_pg_cfg usr_mmu_cfg_from_flags(uint32_t flags)
{
    switch (flags & 0b111U) {
        case BIT(F_READ) | BIT(F_WRITE):
            return mmu_pg_cfg_new(
                0,
                MMU_AP_EL0_RW_EL1_RW,
                MMU_SH_INNER_SHAREABLE,
                false,
                true,
                true,
                true,
                0);

        case BIT(F_READ):
            return mmu_pg_cfg_new(
                0,
                MMU_AP_EL0_RO_EL1_RO,
                MMU_SH_INNER_SHAREABLE,
                false,
                true,
                true,
                true,
                0);

        case BIT(F_READ) | BIT(F_EXEC):
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
static void push_usr_region_to_task(utask* utask, usr_region region)
{
    uintptr_t new_start = region.any.usr_start;

    usr_region_node* prev = NULL;
    usr_region_node* cur = utask->regions;

    while (cur && cur->region.any.usr_start < new_start) {
        prev = cur;
        cur = cur->next;
    }

#ifdef DEBUG
    uintptr_t new_end = region.any.usr_start + region.any.pages * KPAGE_SIZE;

    // check overlap with prev
    if (prev) {
        uintptr_t prev_end =
            prev->region.any.usr_start + prev->region.any.pages * KPAGE_SIZE;

        if (prev_end > new_start)
            PANIC("user regions overlaps");
    }

    // check overlap with next
    if (cur) {
        uintptr_t cur_start = cur->region.any.usr_start;

        if (new_end > cur_start)
            PANIC("user regions overlaps");
    }
#endif

    usr_region_node* node = kmalloc(sizeof(usr_region_node));
    node->region = region;
    node->next = cur;

    if (prev)
        prev->next = node;
    else
        utask->regions = node;
}


static void umalloc_subregion(
    mmu_mapping* mapping,
    usr_region* region,
    const char* tag,
    uintptr_t subregion_start,
    uint32_t pages)
{
    DEBUG_ASSERT(region->any.knl_start != 0);
    DEBUG_ASSERT(
        region->any.usr_start <= subregion_start &&
            region->any.usr_start + region->any.pages * KPAGE_SIZE >=
                subregion_start + pages * KPAGE_SIZE,
        "umalloc_subregion: subregion out of region");


    uintptr_t knl_subregion_start =
        region->any.knl_start + (subregion_start - region->any.usr_start);

    uint32_t remaining_pages = pages;

    while (remaining_pages) {
        uintptr_t offset = (pages - remaining_pages) * KPAGE_SIZE;
        size_t order = log2_floor(remaining_pages);

        puintptr_t pa = page_malloc(order, mm_page_data_new(tag, false, false));

        mmu_map_result mres;
        // map kernel access
        mres = mmu_map(
            MM_MMU_KERNEL_MAPPING,
            knl_subregion_start + offset,
            pa,
            power_of2(order) * KPAGE_SIZE,
            KNL_MMU_CFG,
            NULL);
        ASSERT(mres == MMU_MAP_OK);

        // map user access
        mres = mmu_map(
            mapping,
            subregion_start + offset,
            pa,
            power_of2(order) * KPAGE_SIZE,
            usr_mmu_cfg_from_flags(region->any.flags),
            NULL);
        ASSERT(mres == MMU_MAP_OK);

        remaining_pages -= power_of2(order);
    }
}


void* umalloc(
    struct utask* t,
    uintptr_t usr_va,
    uint32_t pages,
    bool read,
    bool write,
    bool execute,
    bool static_lifetime)
{
    ASSERT(pages > 0);


    // if pages > 64 the assigned pa bitfield is saved as an allocated array of
    // bitfielf64, else it is just one bitfield64
    usr_region ur = (usr_region) {
        .any =
            {
                .pages = pages,
                .flags = 0,
                .usr_start = usr_va,
                .knl_start = 0,
                ._ = 0, // NULL or 0 bitfield64
            },
    };


    SET_FLAG(ur.any.flags, F_READ, read);
    SET_FLAG(ur.any.flags, F_WRITE, write);
    SET_FLAG(ur.any.flags, F_EXEC, execute);
    SET_FLAG(ur.any.flags, F_STATIC_LIFETIME, static_lifetime);
    SET_FLAG(ur.any.flags, F_FULL_MAPPED, static_lifetime);
    SET_FLAG(ur.any.flags, F_PARTIALLY_MAPPED, false);


    if (!static_lifetime) {
        push_usr_region_to_task(t, ur);
        return NULL;
    }


    raw_kmalloc_info kinfo;


    bitfield64* assigned_pa;

    if (pages > 64) {
        // big
        ur.bg.pt_assigned_pa = kmalloc(
            DIV_CEIL(pages, BITFIELD_CAPACITY(bitfield64)) *
            sizeof(bitfield64));

        assigned_pa = ur.bg.pt_assigned_pa;
    }
    else // small
        assigned_pa = &ur.sm.assigned_pa;


    // allocate the kernel access and assign the pa
    ur.any.knl_start = (vuintptr_t)
        raw_kmalloc(pages, t->task_name, &RAW_KMALLOC_DYNAMIC_CFG, &kinfo);


    size_t n = vmalloc_get_pa_count(kinfo.info.dynamic.vtoken);
    vmalloc_pa_info pa_info[n];

    bool res = vmalloc_get_pa_info(kinfo.info.dynamic.vtoken, pa_info, n);
    ASSERT(res);

    // map each allocated phys addresses from the kernel access to the user va
    for (size_t i = 0; i < n; i++) {
        // pa_info comes ordered by va
        size_t offset = pa_info[i].va - pa_info[0].va;

        mmu_map_result mres = mmu_map(
            &t->mapping,
            usr_va + offset,
            pa_info[i].pa,
            power_of2(pa_info[i].order) * KPAGE_SIZE,
            usr_mmu_cfg_from_flags(ur.any.flags),
            NULL);

        ASSERT(mres == MMU_MAP_OK);


        // mark the pages as allocated
        size_t count = power_of2(pa_info[i].order);
        size_t idx = offset / KPAGE_SIZE;

        for (size_t j = 0; j < count; j++) {
            size_t bf_n = (idx + j) / 64;
            size_t bf_i = (idx + j) % 64;
#ifdef DEBUG
            if (ur.any.pages <= 64)
                DEBUG_ASSERT(bf_n == 0);
#endif
            bitfield_set_high(assigned_pa[bf_n], bf_i);
        }
    }

    push_usr_region_to_task(t, ur);

    return (void*)ur.any.knl_start;
}


void* umalloc_assign_pa(struct utask* t, uintptr_t usr_va, uint32_t pages)
{
    // find the usr va node
    usr_region_node* cur = t->regions;

    ASSERT(cur, "umalloc_assign_pa: task has no allocated regions");

    uintptr_t start, end, cur_start, cur_end;
    start = usr_va;
    end = usr_va + pages * KPAGE_SIZE;

    while (cur) {
        cur_start = cur->region.any.usr_start;
        cur_end =
            cur->region.any.usr_start + cur->region.any.pages * KPAGE_SIZE;

        if (cur_start <= start && cur_end >= end)
            break;

        cur = cur->next;
    }

    ASSERT(
        cur,
        "umalloc_assign: no allocated region matches with the requested "
        "subregion");
    ASSERT(!GET_FLAG(cur->region.any.flags, F_FULL_MAPPED));


    // if the region has no subregions assigned, it does not have a kernel
    // access to the region allocated with vmalloc
    if (cur->region.any.knl_start == 0) {
        // reserve the kernel virtual region (all the region, not only the
        // subregion)
        vmalloc_token vtoken;
        cur->region.any.knl_start = vmalloc(
            cur->region.any.pages,
            "usr task region kernel access",
            (vmalloc_cfg) {
                .kmap =
                    {
                        .use_kmap = false,
                        .kmap_pa = 0,
                    },
                .assing_pa = false,
                .device_mem = false,
                .permanent = false,
            },
            &vtoken);
    }

    umalloc_subregion(&t->mapping, &cur->region, t->task_name, usr_va, pages);

    size_t offset = usr_va - cur->region.any.usr_start;

    return (void*)(cur->region.any.knl_start + offset);
}


void ufree(struct utask* t, uintptr_t usr_va)
{
    usr_region_node* cur = t->regions;
    usr_region_node* prev = NULL;

    while (cur) {
        if (cur->region.any.usr_start == usr_va) {
            size_t pages = cur->region.any.pages;

            if (prev)
                prev->next = cur->next;
            else
                t->regions = cur->next;

            mmu_unmap_result ures =
                mmu_unmap(&t->mapping, usr_va, pages * KPAGE_SIZE, NULL);
            ASSERT(ures);

            // free the allocated bitfield64 array if it is a big region
            if (cur->region.any.pages > 64 &&
                cur->region.bg.pt_assigned_pa != NULL)
                kfree(cur->region.bg.pt_assigned_pa);

            kfree(cur);

            return;
        }

        prev = cur;
        cur = cur->next;
    }

    PANIC("ufree: region not found");
}


bool uregion_is_allocated(
    struct utask* t,
    uintptr_t start,
    size_t size,
    usr_region** region)
{
    if (size == 0)
        return false;

    uintptr_t end = start + size;

    usr_region_node* cur = t->regions;

    while (cur) {
        uintptr_t r_start = cur->region.any.usr_start;
        uintptr_t r_end = r_start + cur->region.any.pages * KPAGE_SIZE;

        if (r_start <= start && r_end >= end) {
            if (region)
                *region = &cur->region;

            return true;
        }

        cur = cur->next;
    }

    return false;
}


bool uregion_is_assigned(
    struct utask* t,
    uintptr_t start,
    size_t size,
    usr_region** region)
{
    usr_region* r;

    if (!uregion_is_allocated(t, start, size, &r))
        return false;

    if (region)
        *region = r;

    if (GET_FLAG(r->any.flags, F_FULL_MAPPED))
        return true;

    if (r->any.knl_start == 0)
        return false;

    uintptr_t r_start = r->any.usr_start;

    size_t first_page = (start - r_start) / KPAGE_SIZE;
    size_t pages = DIV_CEIL(size, KPAGE_SIZE);

    bitfield64* assigned_pa;

    if (r->any.pages > 64)
        assigned_pa = r->bg.pt_assigned_pa;
    else
        assigned_pa = &r->sm.assigned_pa;

    for (size_t i = 0; i < pages; i++) {
        size_t idx = first_page + i;
        size_t bf_n = idx / 64;
        size_t bf_i = idx % 64;

        if (!bitfield_get(assigned_pa[bf_n], bf_i))
            return false;
    }

    return true;
}