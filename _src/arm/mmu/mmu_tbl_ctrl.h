#pragma once
#include <arm/mmu.h>
#include <lib/math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/panic.h"
#include "lib/mem.h"
#include "mmu_dc.h"
#include "mmu_types.h"


static inline mmu_pg_cfg cfg_from_dc(mmu_hw_dc dc)
{
    return (mmu_pg_cfg) {
        .attr_index = dc_get_attr_index(dc),
        .ap = dc_get_access_permissions(dc),
        .shareability = dc_get_shareability(dc),
        .non_secure = dc_get_non_secure(dc),
        .access_flag = dc_get_access_flag(dc),
        .pxn = dc_get_privileged_execute_never(dc),
        .uxn = dc_get_unprivileged_execute_never(dc),
        .sw = dc_get_software_defined(dc),
    };
}


static inline size_t level_shift_(mmu_granularity g, mmu_tbl_level l)
{
    size_t page_bits = log2_floor_u64(g); // 12 / 14 / 16
    size_t index_bits = page_bits - 3;

    size_t max_level = (g == MMU_GRANULARITY_64KB) ? MMU_TBL_LV2 : MMU_TBL_LV3;

    DEBUG_ASSERT(l <= max_level);

    return page_bits + index_bits * (max_level - l);
}


static inline size_t tbl_entries(mmu_granularity g)
{
    return g / sizeof(mmu_hw_dc);
}


static inline mmu_tbl
tbl_from_td(const mmu_mapping* m, mmu_hw_dc dc, mmu_tbl_level l)
{
    ASSERT(dc_get_type(dc, m->g_, l) == MMU_DESCRIPTOR_TABLE);

    p_uintptr_t pa = dc_get_output_address(dc, m->g_);
    v_uintptr_t va = pa + m->physmap_offset_;

    DEBUG_ASSERT(pa && pa % m->g_ == 0);

    return (mmu_tbl) {
        .dcs = (mmu_hw_dc*)va,
    };
}


static inline size_t
table_index(p_uintptr_t va, mmu_granularity g, mmu_tbl_level l)
{
    size_t index_bits = log2_floor_u64(g) - 3;
    size_t mask = (1ULL << index_bits) - 1;

    return (va >> level_shift_(g, l)) & mask;
}


static inline size_t dc_cover_bytes(mmu_granularity g, mmu_tbl_level l)
{
    size_t page_bits = log2_floor_u64(g);
    size_t index_bits = page_bits - 3;

    ASSERT(l <= max_level(g));

    return 1ULL << (page_bits + index_bits * (max_level(g) - l));
}


static inline void tbl_init_null(mmu_tbl tbl, mmu_granularity g)
{
    DEBUG_ASSERT((uintptr_t)tbl.dcs % g == 0);

    memzero64(tbl.dcs, tbl_entries(g) * sizeof(mmu_hw_dc));
}


static inline mmu_tbl
alloc_tbl(const mmu_mapping* m, bool init_null, mmu_op_info* info)
{
    if (info)
        info->alocated_tbls++;

    mmu_tbl tbl = (mmu_tbl) {
        .dcs = m->allocator_(sizeof(mmu_hw_dc) * tbl_entries(m->g_)),
    };

    ASSERT((v_uintptr_t)tbl.dcs % m->g_ == 0);

    if (init_null)
        tbl_init_null(tbl, m->g_);

    return tbl;
}


/// divides a block into a next level table and udcates the parent. Returns the
/// created table (of a lower level)
static inline mmu_tbl split_block(
    const mmu_mapping* m,
    mmu_tbl parent,
    size_t index,
    mmu_tbl_level l,
    mmu_op_info* info)
{
    const mmu_hw_dc old = parent.dcs[index];
    const mmu_tbl new_tbl = alloc_tbl(m, false, info);
    const mmu_granularity g = m->g_;

    DEBUG_ASSERT(l < max_level(g));
    DEBUG_ASSERT(dc_get_type(old, g, l) == MMU_DESCRIPTOR_BLOCK);

    // create the new blocks
    if (dc_get_valid(old)) {
        mmu_pg_cfg cfg = cfg_from_dc(old);
        p_uintptr_t pa = dc_get_output_address(old, g);
        size_t new_l_bytes = dc_cover_bytes(g, l + 1);
        ASSERT(pa % dc_cover_bytes(g, l) == 0);

        for (size_t i = 0; i < tbl_entries(g); i++)
            new_tbl.dcs[i] = bd_build(cfg, pa + (i * new_l_bytes), g, l + 1);
    }
    else {
        tbl_init_null(new_tbl, g);
    }


    // set the new table
    parent.dcs[index] = td_build(m, new_tbl);

    return new_tbl;
}


static inline bool tbl_is_null(mmu_tbl tbl, mmu_granularity g)
{
    for (size_t i = 0; i < tbl_entries(g); i++)
        if (dc_get_valid(tbl.dcs[i]))
            return false;

    return true;
}


static inline void
free_tbl(const mmu_mapping* m, mmu_tbl tbl, mmu_tbl_level l, mmu_op_info* info)
{
    for (size_t i = 0; i < tbl_entries(m->g_); i++)
        if (dc_get_valid(tbl.dcs[i]) &&
            dc_get_type(tbl.dcs[i], m->g_, l) == MMU_DESCRIPTOR_TABLE)
            free_tbl(m, tbl_from_td(m, tbl.dcs[i], l + 1), l + 1, info);


    m->allocator_free_(tbl.dcs);

    if (info)
        info->freed_tbls++;
}


static inline mmu_tbl mmu_mapping_get_tbl(const mmu_mapping* m)
{
    return (mmu_tbl) {
        .dcs = m->tbl_,
    };
}
