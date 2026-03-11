#pragma once

/*
 *  mmu descriptors
 */

#include <arm/mmu.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>

#include "mmu_types.h"


#define NULL_PD (mmu_hw_dc) {.v = 0}


static inline mmu_hw_dc mmu_tbl_get_dc(mmu_tbl tbl, size_t i, mmu_granularity g)
{
    (void)g;
    DEBUG_ASSERT(tbl.dcs, "null provided table");
    DEBUG_ASSERT(i < (g / sizeof(mmu_hw_dc)), "idx out of tbl granularity");

    return tbl.dcs[i];
}


static inline uint64_t mmu_granularity_shift(mmu_granularity g)
{
    switch (g) {
        case MMU_GRANULARITY_4KB:
            return 12;
        case MMU_GRANULARITY_16KB:
            return 14;
        case MMU_GRANULARITY_64KB:
            return 16;
#ifdef TEST
        default:
            PANIC("mmu_granularity_shift unhandled");
#endif
    }
    return 12;
}


static inline uint64_t output_address_bit_n_(mmu_granularity g)
{
    const uint64_t shift = mmu_granularity_shift(g);
    const uint64_t pa_bit_n = 48 - shift;

    return pa_bit_n;
}


static inline uint64_t output_address_mask_(mmu_granularity g)
{
    const uint64_t shift = mmu_granularity_shift(g);
    const uint64_t pa_bit_n = 48 - shift;

    return ((1ULL << pa_bit_n) - 1) << shift;
}


/*
 *  Page descriptor bit definitions
 */
#define MMU_DC_VALID_SHIFT 0
#define MMU_DC_VALID_WIDTH 1

#define MMU_DC_TYPE_SHIFT 1
#define MMU_DC_TYPE_WIDTH 1

#define MMU_DC_ATTR_INDEX_SHIFT 2
#define MMU_DC_ATTR_INDEX_WIDTH 3

#define MMU_DC_NS_SHIFT 5
#define MMU_DC_NS_WIDTH 1

#define MMU_DC_AP_SHIFT 6
#define MMU_DC_AP_WIDTH 2

#define MMU_DC_SH_SHIFT 8
#define MMU_DC_SH_WIDTH 2

#define MMU_DC_AF_SHIFT 10
#define MMU_DC_AF_WIDTH 1

#define MMU_DC_PXN_SHIFT 53
#define MMU_DC_PXN_WIDTH 1

#define MMU_DC_UXN_SHIFT 54
#define MMU_DC_UXN_WIDTH 1

#define MMU_DC_SW_SHIFT 55
#define MMU_DC_SW_WIDTH 4


#define MMU_DC_OUTPUT_ADDR_MASK 0x0000FFFFFFFFF000ULL


/* helpers */
#define MMU_DC_BITS(width) ((1ULL << (width)) - 1)
#define MMU_DC_FIELD_MASK(shift, width) (MMU_DC_BITS(width) << (shift))


// getters
static inline bool dc_get_valid(const mmu_hw_dc dc)
{
    return (bool)((dc.v >> MMU_DC_VALID_SHIFT) &
                  MMU_DC_BITS(MMU_DC_VALID_WIDTH));
}

static inline mmu_descriptor_type
dc_get_type(const mmu_hw_dc dc, mmu_granularity g, mmu_tbl_level l)
{
    DEBUG_ASSERT(l <= max_level(g));

    uint64_t type_bit =
        (dc.v >> MMU_DC_TYPE_SHIFT) & MMU_DC_BITS(MMU_DC_TYPE_WIDTH);

    if (l == max_level(g)) {
        ASSERT(
            type_bit == 1,
            "descriptor_get_type: only page descriptors are valid");
        return MMU_DESCRIPTOR_PAGE;
    }

    return type_bit == 0 ? MMU_DESCRIPTOR_BLOCK : MMU_DESCRIPTOR_TABLE;
}

static inline uint8_t dc_get_attr_index(const mmu_hw_dc dc)
{
    return (uint8_t)((dc.v >> MMU_DC_ATTR_INDEX_SHIFT) &
                     MMU_DC_BITS(MMU_DC_ATTR_INDEX_WIDTH));
}

static inline bool dc_get_non_secure(const mmu_hw_dc dc)
{
    return (bool)((dc.v >> MMU_DC_NS_SHIFT) & MMU_DC_BITS(MMU_DC_NS_WIDTH));
}

static inline mmu_access_permission
dc_get_access_permissions(const mmu_hw_dc dc)
{
    return (mmu_access_permission)((dc.v >> MMU_DC_AP_SHIFT) &
                                   MMU_DC_BITS(MMU_DC_AP_WIDTH));
}

static inline mmu_shareability dc_get_shareability(const mmu_hw_dc dc)
{
    return (mmu_shareability)((dc.v >> MMU_DC_SH_SHIFT) &
                              MMU_DC_BITS(MMU_DC_SH_WIDTH));
}

static inline bool dc_get_access_flag(const mmu_hw_dc dc)
{
    return (bool)((dc.v >> MMU_DC_AF_SHIFT) & MMU_DC_BITS(MMU_DC_AF_WIDTH));
}

static inline p_uintptr_t
dc_get_output_address(const mmu_hw_dc dc, mmu_granularity g)
{
    return dc.v & output_address_mask_(g);
}

static inline bool dc_get_privileged_execute_never(const mmu_hw_dc dc)
{
    return (bool)((dc.v >> MMU_DC_PXN_SHIFT) & MMU_DC_BITS(MMU_DC_PXN_WIDTH));
}

static inline bool dc_get_unprivileged_execute_never(const mmu_hw_dc dc)
{
    return (bool)((dc.v >> MMU_DC_UXN_SHIFT) & MMU_DC_BITS(MMU_DC_UXN_WIDTH));
}

static inline uint8_t dc_get_software_defined(const mmu_hw_dc dc)
{
    return (uint8_t)((dc.v >> MMU_DC_SW_SHIFT) & MMU_DC_BITS(MMU_DC_SW_WIDTH));
}


// setters
static inline void dc_set_valid(mmu_hw_dc* dc, bool valid)
{
    dc->v &= ~MMU_DC_FIELD_MASK(MMU_DC_VALID_SHIFT, MMU_DC_VALID_WIDTH);
    dc->v |= ((uint64_t)valid << MMU_DC_VALID_SHIFT);
}

static inline void dc_set_type(mmu_hw_dc* dc, mmu_descriptor_type type)
{
    uint64_t type_bit = type == MMU_DESCRIPTOR_BLOCK ? 0ULL : 1ULL;

    dc->v &= ~MMU_DC_FIELD_MASK(MMU_DC_TYPE_SHIFT, MMU_DC_TYPE_WIDTH);
    dc->v |= (type_bit << MMU_DC_TYPE_SHIFT);
}

static inline void dc_set_attr_index(mmu_hw_dc* dc, uint8_t attr_index)
{
    dc->v &=
        ~MMU_DC_FIELD_MASK(MMU_DC_ATTR_INDEX_SHIFT, MMU_DC_ATTR_INDEX_WIDTH);
    dc->v |= ((uint64_t)attr_index & MMU_DC_BITS(MMU_DC_ATTR_INDEX_WIDTH))
             << MMU_DC_ATTR_INDEX_SHIFT;
}

static inline void dc_set_non_secure(mmu_hw_dc* dc, bool non_secure)
{
    dc->v &= ~MMU_DC_FIELD_MASK(MMU_DC_NS_SHIFT, MMU_DC_NS_WIDTH);
    dc->v |= ((uint64_t)non_secure << MMU_DC_NS_SHIFT);
}

static inline void
dc_set_access_permissions(mmu_hw_dc* dc, mmu_access_permission permissions)
{
    dc->v &= ~MMU_DC_FIELD_MASK(MMU_DC_AP_SHIFT, MMU_DC_AP_WIDTH);
    dc->v |= ((uint64_t)permissions << MMU_DC_AP_SHIFT);
}

static inline void
dc_set_shareability(mmu_hw_dc* dc, mmu_shareability shareability)
{
    dc->v &= ~MMU_DC_FIELD_MASK(MMU_DC_SH_SHIFT, MMU_DC_SH_WIDTH);
    dc->v |= ((uint64_t)shareability & MMU_DC_BITS(MMU_DC_SH_WIDTH))
             << MMU_DC_SH_SHIFT;
}

static inline void dc_set_access_flag(mmu_hw_dc* dc, bool access_flag)
{
    dc->v &= ~MMU_DC_FIELD_MASK(MMU_DC_AF_SHIFT, MMU_DC_AF_WIDTH);
    dc->v |= ((uint64_t)access_flag << MMU_DC_AF_SHIFT);
}

static inline void
dc_set_output_address(mmu_hw_dc* dc, p_uintptr_t output_address)
{
    dc->v &= ~MMU_DC_OUTPUT_ADDR_MASK;
    dc->v |= output_address & MMU_DC_OUTPUT_ADDR_MASK;
}


static inline void dc_set_privileged_execute_never(mmu_hw_dc* dc, bool pxn)
{
    dc->v &= ~MMU_DC_FIELD_MASK(MMU_DC_PXN_SHIFT, MMU_DC_PXN_WIDTH);
    dc->v |= ((uint64_t)pxn << MMU_DC_PXN_SHIFT);
}

static inline void dc_set_unprivileged_execute_never(mmu_hw_dc* dc, bool uxn)
{
    dc->v &= ~MMU_DC_FIELD_MASK(MMU_DC_UXN_SHIFT, MMU_DC_UXN_WIDTH);
    dc->v |= ((uint64_t)uxn << MMU_DC_UXN_SHIFT);
}

static inline void
dc_set_software_defined(mmu_hw_dc* dc, uint8_t software_defined)
{
    dc->v &= ~MMU_DC_FIELD_MASK(MMU_DC_SW_SHIFT, MMU_DC_SW_WIDTH);
    dc->v |= ((uint64_t)software_defined & MMU_DC_BITS(MMU_DC_SW_WIDTH))
             << MMU_DC_SW_SHIFT;
}


static inline mmu_hw_dc td_build(const mmu_mapping* m, mmu_tbl next)
{
    mmu_hw_dc dc = (mmu_hw_dc) {0};

    p_uintptr_t tbl_pa = (v_uintptr_t)next.dcs - m->physmap_offset_;

    dc_set_type(&dc, MMU_DESCRIPTOR_TABLE);
    dc_set_output_address(&dc, tbl_pa);
    dc_set_valid(&dc, true);

    return dc;
}

static inline mmu_hw_dc bd_build(
    mmu_pg_cfg cfg,
    p_uintptr_t output_address,
    mmu_granularity g,
    mmu_tbl_level l)
{
    DEBUG_ASSERT(l <= max_level(g));

    mmu_hw_dc dc = (mmu_hw_dc) {0};

    dc_set_valid(&dc, true);
    dc_set_type(
        &dc,
        l == max_level(g) ? MMU_DESCRIPTOR_PAGE : MMU_DESCRIPTOR_BLOCK);
    dc_set_attr_index(&dc, cfg.attr_index);
    dc_set_non_secure(&dc, cfg.non_secure);
    dc_set_access_permissions(&dc, cfg.ap);
    dc_set_shareability(&dc, cfg.shareability);
    dc_set_access_flag(&dc, cfg.access_flag);
    dc_set_output_address(&dc, output_address);
    dc_set_privileged_execute_never(&dc, cfg.pxn);
    dc_set_unprivileged_execute_never(&dc, cfg.uxn);
    dc_set_software_defined(&dc, cfg.sw);

    return dc;
}
