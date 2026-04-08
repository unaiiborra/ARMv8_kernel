#include "raw_kmalloc.h"

#include <arm/mmu.h>
#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/mm/page_malloc.h>
#include <kernel/mm/vmalloc.h>
#include <kernel/panic.h>
#include <lib/lock.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdmacros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../internal/reserve_malloc.h"


const raw_kmalloc_cfg RAW_KMALLOC_KMAP_CFG = (raw_kmalloc_cfg) {
    .assign_pa    = true,
    .fill_reserve = true,
    .device_mem   = false,
    .permanent    = false,
    .kmap         = true,
    .init_zeroed  = false,
};

const raw_kmalloc_cfg RAW_KMALLOC_DYNAMIC_CFG = (raw_kmalloc_cfg) {
    .assign_pa    = true,
    .fill_reserve = true,
    .device_mem   = false,
    .permanent    = false,
    .kmap         = false,
    .init_zeroed  = false,
};


static const mmu_pg_cfg STD_MMU_KMEM_CFG = (mmu_pg_cfg) {
    .attr_index   = 0,
    .ap           = MMU_AP_EL0_NONE_EL1_RW,
    .shareability = MMU_SH_INNER_SHAREABLE,
    .non_secure   = false,
    .access_flag  = 1,
    .pxn          = 0,
    .uxn          = 1,
    .sw           = 0,
};

static const mmu_pg_cfg STD_MMU_DEVICE_CFG = (mmu_pg_cfg) {
    .attr_index   = 1,
    .ap           = MMU_AP_EL0_NONE_EL1_RW,
    .shareability = MMU_SH_NON_SHAREABLE,
    .non_secure   = false,
    .access_flag  = 1,
    .pxn          = 1,
    .uxn          = 1,
    .sw           = 0,
};


static corelock_t lock;


static inline vmalloc_cfg
vmalloc_cfg_from_raw_kmalloc_cfg(const raw_kmalloc_cfg* cfg, puintptr_t kmap_pa)
{
    return (vmalloc_cfg) {
        .assing_pa  = cfg->assign_pa,
        .device_mem = cfg->device_mem,
        .permanent  = cfg->permanent,
        .kmap =
            {
                .use_kmap = cfg->kmap,
                .kmap_pa  = cfg->kmap ? kmap_pa : 0,
            },
    };
}


static void* raw_kmalloc_kmap(
    size_t                 pages,
    const char*            tag,
    const raw_kmalloc_cfg* cfg,
    raw_kmalloc_info*      info)
{
    DEBUG_ASSERT(cfg->kmap && cfg->assign_pa);

    ASSERT(is_pow2(pages), "only pow2 n pages can be kmapped");

    size_t o = log2_floor(pages);

    puintptr_t pa = page_malloc(
        o,
        (mm_page_data) {
            .tag        = tag,
            .device_mem = cfg->device_mem,
            .permanent  = cfg->permanent,
        });

    vuintptr_t va =
        vmalloc(pages, tag, vmalloc_cfg_from_raw_kmalloc_cfg(cfg, pa), NULL);

    DEBUG_ASSERT(ptrs_are_kmapped(pv_ptr_new(pa, va)));

    const mmu_pg_cfg* mmu_cfg =
        cfg->device_mem ? &STD_MMU_DEVICE_CFG : &STD_MMU_KMEM_CFG;

    mmu_map_result mmu_res = mmu_map(
        MM_MMU_KERNEL_MAPPING,
        va,
        pa,
        pages * PAGE_SIZE,
        *mmu_cfg,
        NULL);
    ASSERT(mmu_res == MMU_MAP_OK);

    if (info) {
        info->raw_kmalloc_type = RAW_KMALLOC_KMAP;
        info->MMU_CFG          = mmu_cfg;
        info->info.kmap.order  = o;
        info->info.kmap.pv     = (pv_ptr) {pa, va};
    }

    return (void*)va;
}


static void* raw_kmalloc_dynamic(
    size_t                 pages,
    const char*            tag,
    const raw_kmalloc_cfg* cfg,
    raw_kmalloc_info*      info)
{
    DEBUG_ASSERT(!cfg->kmap);
    ASSERT(cfg->assign_pa, "vmalloc: TODO: NOT IMPLEMENTED YET");

    const mmu_pg_cfg* mmu_cfg =
        cfg->device_mem ? &STD_MMU_DEVICE_CFG : &STD_MMU_KMEM_CFG;

    vmalloc_token vtoken;
    vuintptr_t    start =
        vmalloc(pages, tag, vmalloc_cfg_from_raw_kmalloc_cfg(cfg, 0), &vtoken);

    vuintptr_t va  = start;
    size_t     rem = pages;

    while (rem > 0) {
        size_t o           = log2_floor(rem);
        size_t order_bytes = power_of2(o) * PAGE_SIZE;

        /*
         *  get phys page
         */
        puintptr_t pa = page_malloc(
            o,
            (mm_page_data) {
                .tag        = tag,
                .device_mem = cfg->device_mem,
                .permanent  = cfg->permanent,
            });

        /*
         *  save in vmalloc the order and corresponding va for that pa
         */
        vmalloc_push_pa(vtoken, o, pa, va);

        /*
         *  mmu map the pages
         */

        bool mmu_res =
            mmu_map(MM_MMU_KERNEL_MAPPING, va, pa, order_bytes, *mmu_cfg, NULL);
        ASSERT(mmu_res == MMU_MAP_OK);


        va += order_bytes;
        rem -= power_of2(o);
    }

    DEBUG_ASSERT(start + (pages * PAGE_SIZE) == va);

    if (info) {
        info->raw_kmalloc_type    = RAW_KMALLOC_DYNAMIC;
        info->MMU_CFG             = mmu_cfg;
        info->info.dynamic.vtoken = vtoken;
    }

    return (void*)start;
}


void raw_kmalloc_init()
{
    corelock_init(&lock);
}


void* __raw_kmalloc(
    size_t                 pages,
    const char*            tag,
    const raw_kmalloc_cfg* cfg,
    raw_kmalloc_info*      info)
{
    void* va;

    cfg = (cfg != NULL) ? cfg : &RAW_KMALLOC_DYNAMIC_CFG;

    ASSERT(cfg->assign_pa, "TODO: dynamic mapping not implemented yet");

    corelocked(&lock)
    {
        if (cfg->kmap)
            va = raw_kmalloc_kmap(pages, tag, cfg, info);
        else
            va = raw_kmalloc_dynamic(pages, tag, cfg, info);

        DEBUG_ASSERT((vuintptr_t)va % PAGE_ALIGN == 0);

        if (cfg->fill_reserve)
            reserve_malloc_fill();
    }

    if (cfg->init_zeroed) {
        memzero64(va, pages * PAGE_SIZE);

#ifdef DEBUG
        uint64_t* ptr = (uint64_t*)va;
        DEBUG_ASSERT((uintptr_t)va % PAGE_SIZE == 0);
        for (size_t i = 0; i < (pages * PAGE_SIZE) / sizeof(uint64_t); i++)
            DEBUG_ASSERT(ptr[i] == 0);
#endif
    }

    return va;
}


void raw_kfree(void* ptr)
{
    vmalloc_token              vtoken;
    vmalloc_allocated_area_mdt vinfo;
    bool                       result;

    corelocked(&lock)
    {
        vtoken = vmalloc_get_token(ptr);
        vinfo  = vmalloc_get_mdt(vtoken);

        if (vinfo.kmapped) {
            size_t bytes = vfree(vtoken, NULL);

            DEBUG_ASSERT(is_pow2(bytes));

            page_free(kva_to_kpa((vuintptr_t)ptr));


            if (vinfo.pa_assigned) {
                result = mmu_unmap(
                    MM_MMU_KERNEL_MAPPING,
                    (vuintptr_t)ptr,
                    bytes,
                    NULL);
                ASSERT(result);
            }
        }
        //  dynamic
        else {
            const size_t N = vmalloc_get_pa_count(vtoken);

            vmalloc_pa_info pages[N];

            result = vmalloc_get_pa_info(vtoken, pages, N);
            ASSERT(result);

            for (size_t i = 0; i < N; i++)
                page_free(pages[i].pa);


            size_t bytes = vfree(vtoken, NULL);

            result =
                mmu_unmap(MM_MMU_KERNEL_MAPPING, (vuintptr_t)ptr, bytes, NULL);
            ASSERT(result);
        }
    }
}


void raw_kmalloc_lock()
{
    core_lock(&lock);
}


void raw_kmalloc_unlock(int*)
{
    core_unlock(&lock);
}
