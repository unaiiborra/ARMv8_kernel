#define __MMU_INTERNAL

#include <arm/mmu.h>
#include <lib/mem.h>
#include <stdbool.h>
#include <stddef.h>

#include "mmu_tbl_ctrl.h"
#include "mmu_types.h"
#include "regs/mmu_sysregs.h"


static inline bool is_in_range(const mmu_mapping* m, vuintptr_t va, size_t size)
{
    if (!m || size == 0)
        return false;

    const uint64_t VA_BITS  = m->va_addr_bits_;
    const uint64_t SIGN_BIT = VA_BITS - 1;

    const uint64_t SIGN_MASK  = 1ULL << SIGN_BIT;
    const uint64_t CANON_MASK = ~((1ULL << VA_BITS) - 1);

    uint64_t sign  = (va >> SIGN_BIT) & 1;
    uint64_t upper = va & CANON_MASK;

    if (sign) {
        if (upper != CANON_MASK)
            return false;
    }
    else {
        if (upper != 0)
            return false;
    }

    vuintptr_t end = va + size - 1;


    if (end < va)
        return false;

    sign  = (end >> SIGN_BIT) & 1;
    upper = end & CANON_MASK;

    if (sign) {
        if (upper != CANON_MASK)
            return false;
    }
    else {
        if (upper != 0)
            return false;
    }

    const bool is_hi  = (va & SIGN_MASK) != 0;
    const bool end_hi = (end & SIGN_MASK) != 0;

    if (is_hi != end_hi)
        return false;

    if (m->rng_ == MMU_LO)
        return !is_hi;

    if (m->rng_ == MMU_HI)
        return is_hi;

    return false;
}


static inline void get_target_lvl(
    mmu_tbl_level*  target_lvl,
    size_t*         cover,
    size_t          size,
    mmu_granularity g,
    vuintptr_t      va,
    puintptr_t      pa)
{
    mmu_tbl_level l;
    size_t        c = 0;

    for (l = MMU_TBL_LV1; l <= max_level(g); l++) {
        c = dc_cover_bytes(g, l);

        if (size >= dc_cover_bytes(g, l) && va % c == 0 && pa % c == 0)
            break;
    }

    if (target_lvl)
        *target_lvl = l;
    if (cover)
        *cover = c;
}


bool mmu_is_active()
{
    return _mmu_get_SCTLR_EL1() & 1ULL;
}


mmu_map_result mmu_map(
    const mmu_mapping* m,
    vuintptr_t         va,
    puintptr_t         pa,
    size_t             size,
    mmu_pg_cfg         cfg,
    mmu_op_info*       info)
{
    mmu_granularity g = m->g_;
    mmu_tbl_level   target_lvl;
    size_t          cover;

    if (!m)
        return MMU_MAP_NULL_MAPPING;

    if (size == 0)
        return MMU_MAP_OK;

    if (!is_in_range(m, va, size))
        return MMU_MAP_MAP_NOT_IN_RANGE;

#ifdef DEBUG
    puintptr_t expected_phys_end = pa + size;
    vuintptr_t expected_virt_end = va + size;
#endif

    const mmu_tbl TBL0 = mmu_mapping_get_tbl(m);

    while (size > 0) {
        size_t  i;
        mmu_tbl tbl = TBL0;

        ASSERT(size % g == 0 && va % g == 0 && pa % g == 0);

        get_target_lvl(&target_lvl, &cover, size, g, va, pa);

        DEBUG_ASSERT(va % cover == 0);
        DEBUG_ASSERT(pa % cover == 0);

        // the main loop of the fn, finds the actual table of the target level
        for (mmu_tbl_level l = MMU_TBL_LV0; l < target_lvl; l++) {
            if (info)
                info->iters += 1;

            i = table_index(va, g, l);

            const mmu_hw_dc descriptor = mmu_tbl_get_dc(tbl, i, g);

            // not valid
            if (!dc_get_valid(descriptor)) {
                // --- invalid descriptor ---
                // allocate a new table
                mmu_tbl next = alloc_tbl(m, true, info);

                // link the new allocated table in the current table level
                tbl.dcs[i] = td_build(m, next);
                tbl        = next;

                continue;
            }

            switch (dc_get_type(descriptor, g, l)) {
                case MMU_DESCRIPTOR_BLOCK:
                    tbl = split_block(m, tbl, i, l, info);
                    continue;
                case MMU_DESCRIPTOR_TABLE:
                    tbl = tbl_from_td(m, descriptor, l);
                    continue;
                default:
                    PANIC("mmu_map: err"); // should not be a block (l <
                                           // target_lvl)
            }
        }

        // build the block descriptor
        i = table_index(va, g, target_lvl);

        mmu_hw_dc old = mmu_tbl_get_dc(tbl, i, g);

        tbl.dcs[i] = bd_build(cfg, pa, g, target_lvl);

        // if it was a table, free it (and all the subtables)
        if (dc_get_valid(old) &&
            dc_get_type(old, g, target_lvl) == MMU_DESCRIPTOR_TABLE)
            free_tbl(m, tbl_from_td(m, old, target_lvl), target_lvl, info);

        size -= cover;
        pa += cover;
        va += cover;
    }

#ifdef DEBUG
    DEBUG_ASSERT(
        size == 0 && pa == expected_phys_end && va == expected_virt_end);
#endif

    MMU_APPLY_CHANGES();

    return MMU_MAP_OK;
}


mmu_unmap_result
mmu_unmap(const mmu_mapping* m, vuintptr_t va, size_t size, mmu_op_info* info)
{
    size_t          cover;
    size_t          i;
    mmu_granularity g = m->g_;
    mmu_tbl_level   l, target_lvl;

    if (size == 0)
        return MMU_UNMAP_OK;

#ifdef DEBUG
    vuintptr_t expected_virt_end = va + size;
#endif

    const mmu_tbl TBL0 = mmu_mapping_get_tbl(m);

    while (size > 0) {
        mmu_tbl tbl = TBL0;

        if (size % g != 0 || va % g != 0)
            return MMU_UNMAP_ERR;

        get_target_lvl(&target_lvl, &cover, size, g, va, 0);


        DEBUG_ASSERT(size % g == 0 || va % g == 0);
        DEBUG_ASSERT(va % cover == 0);


        // the main loop of the fn, finds the actual table of the target level
        bool already_unmapped = false;

        for (l = MMU_TBL_LV0; l < target_lvl; l++) {
            if (info)
                info->iters += 1;

            i = table_index(va, g, l);

            mmu_hw_dc dc = mmu_tbl_get_dc(tbl, i, g);

            // not valid
            if (!dc_get_valid(dc)) {
                already_unmapped = true;
                break;
            }

            switch (dc_get_type(dc, g, l)) {
                case MMU_DESCRIPTOR_BLOCK:
                    tbl = split_block(m, tbl, i, l, info);
                    continue;
                case MMU_DESCRIPTOR_TABLE:
                    tbl = tbl_from_td(m, dc, l);
                    continue;
                default:
                    break;
            }

            PANIC("mmu_map: err");
        }

        if (already_unmapped) {
            cover = min(size, dc_cover_bytes(g, l));
            size -= cover;
            va += cover;
            continue;
        }

        // build the null block descriptor
        i             = table_index(va, g, target_lvl);
        mmu_hw_dc old = mmu_tbl_get_dc(tbl, i, g);

        tbl.dcs[i] = NULL_PD;

        // if it was a table, free it (and all the subtables)
        if (dc_get_type(old, g, target_lvl) == MMU_DESCRIPTOR_TABLE &&
            dc_get_valid(old))
            free_tbl(m, tbl_from_td(m, old, target_lvl), target_lvl, info);

        size -= cover;
        va += cover;
    }


    // TODO: Collapse null tables
    /*
     *  tbl = tbl0;
     *  // another final look to see if a table is fully null and collapse it
     *  for (l = MMU_TBL_LV0; l <= max_level(g); l++) {
     *      i = table_index(virt, g, l);
     *
     *      mmu_hw_dc dc = tbl.dcs[i];
     *
     *      if (!dc_get_valid(dc))
     *          break;
     *
     *      if (dc_get_type(dc) == MMU_PD_TABLE) {
     *          mmu_tbl subtbl = tbl_from_td(dc, g);
     *
     *          if (tbl_is_null(subtbl, g)) {
     *              tbl.dcs[i] = NULL_PD;
     *
     *              UNLOCKED_free_subtree(h, tbl_from_td(dc, g), info);
     *              break;
     *          }
     *
     *          tbl = subtbl;
     *      }
     *  }
     */

#ifdef DEBUG
    DEBUG_ASSERT(size == 0 && va == expected_virt_end);
#endif

    return MMU_UNMAP_OK;
}
