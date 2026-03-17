#pragma once

// https://github.com/bztsrc/raspi3-tutorial/blob/master/10_virtualmemory/mmu.c
// https://documentation-service.arm.com/static/63a43e333f28e5456434e18b?mapping=

#include <kernel/panic.h>
#include <lib/lock.h>
#include <lib/mem.h>
#include <lib/unit/mem.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/*
 *  MMU Core
 */

/// This defines are used for the inline functions only, they are not part of
/// the header exposed api. They are undefined later
#define COREH_DCACHE_SHIFT 0
#define COREH_ICACHE_SHIFT 1
#define COREH_ALIGN_TRAP_SHIFT 2
#define COREH_LO_ENABLE_SHIFT 3
#define COREH_HI_ENABLE_SHIFT 4
#define COREH_LO_BITS_SHIFT 5         // 10:5 6 bit width
#define COREH_HI_BITS_SHIFT 11        // 16:11 6 bit width
#define COREH_LO_GRANULARITY_SHIFT 17 // 17:12
#define COREH_HI_GRANULARITY_SHIFT 19 // 19:18


#define COREH_VA_BITS_WIDTH 6
#define COREH_VA_BITS_MASK ((1ull << COREH_VA_BITS_WIDTH) - 1)
#define COREH_GRANULARITY_WIDTH 2
#define COREH_GRANULARITY_MASK ((1ull << COREH_GRANULARITY_WIDTH) - 1)
#define COREH_BIT_MASK(shift) (1ull << (shift))
#define MMU_APPLY_CHANGES()       \
    asm volatile("dsb ishst\n"    \
                 "tlbi vmalle1\n" \
                 "dsb ish\n"      \
                 "isb\n");


// TCR_EL1.TG0 encoding
#define TG0_4KB 0b00
#define TG0_16KB 0b10
#define TG0_64KB 0b01
// TCR_EL1.TG1 encoding
#define TG1_4KB 0b10
#define TG1_16KB 0b01
#define TG1_64KB 0b11

#define NULL_MAPPING_TBL (void*)~0ULL

typedef enum {
    MMU_GRANULARITY_4KB = 4 * MEM_KiB,
    MMU_GRANULARITY_16KB = 16 * MEM_KiB,
    MMU_GRANULARITY_64KB = 64 * MEM_KiB,
} mmu_granularity;

typedef enum {
    MMU_LO = 0,
    MMU_HI = 1,
} mmu_tbl_rng;

/// the return type must be a virtual address that is mapped according to the
/// provided mmu physmap offset
typedef void* (*mmu_allocator)(size_t bytes);
typedef void (*mmu_allocator_free)(void* addr);
typedef uint16_t (*mmu_coreid)(void);

typedef struct {
    mmu_tbl_rng rng_;
    mmu_granularity g_;
    size_t va_addr_bits_;
    uint64_t physmap_offset_;
    void* tbl_;
    mmu_allocator allocator_;
    mmu_allocator_free allocator_free_;
} mmu_mapping;

extern const mmu_mapping MMU_NULL_MAPPING;


static inline mmu_tbl_rng mmu_mapping_get_rng(const mmu_mapping* m)
{
    return m->rng_;
}

static inline mmu_granularity mmu_mapping_get_granularity(const mmu_mapping* m)
{
    return m->g_;
}

static inline size_t mmu_mapping_get_va_addr_bits(const mmu_mapping* m)
{
    return m->va_addr_bits_;
}

static inline uint64_t mmu_mapping_get_physmap_offset(const mmu_mapping* m)
{
    return m->physmap_offset_;
}

static inline void* mmu_mapping_get_table(const mmu_mapping* m)
{
    return m->tbl_;
}

static inline mmu_allocator mmu_mapping_get_allocator(const mmu_mapping* m)
{
    return m->allocator_;
}

static inline mmu_allocator_free
mmu_mapping_get_allocator_free(const mmu_mapping* m)
{
    return m->allocator_free_;
}

static inline bool mmu_mapping_is_valid(const mmu_mapping* m)
{
    if (m == NULL)
        return false;

    return m->tbl_ != NULL_MAPPING_TBL;
}

static inline void
mmu_mapping_set_physmap_offset(mmu_mapping* m, uint64_t physmap_offset)
{
    m->physmap_offset_ = physmap_offset;
}

static inline void
mmu_mapping_set_allocator(mmu_mapping* m, mmu_allocator allocator)
{
    m->allocator_ = allocator;
}

static inline void
mmu_mapping_set_allocator_free(mmu_mapping* m, mmu_allocator_free allocator_free)
{
    m->allocator_free_ = allocator_free;
}

static inline void
UNSAFE_mmu_mapping_set_tbl_address(mmu_mapping* m, void* addr)
{
    m->tbl_ = addr;
}

typedef struct {
    mmu_mapping* lo_mapping;
    mmu_mapping* hi_mapping;
    uint32_t mpidr_aff;
    uint64_t flags;
} mmu_core_handle;


bool mmu_core_handle_new(
    mmu_core_handle* out,
    mmu_mapping* lo_mapping,
    mmu_mapping* hi_mapping,
    bool hi_enable,
    bool lo_enable,
    bool d_cache,
    bool i_cache,
    bool align_trap);


void mmu_delete_mapping(mmu_mapping* m);


static inline mmu_mapping* mmu_core_get_lo_mapping(const mmu_core_handle* ch)
{
    return ch->lo_mapping;
}

static inline mmu_mapping* mmu_core_get_hi_mapping(const mmu_core_handle* ch)
{
    return ch->hi_mapping;
}

static inline bool mmu_core_get_d_cache(const mmu_core_handle* ch)
{
    return (ch->flags >> COREH_DCACHE_SHIFT) & 1ULL;
}

static inline bool mmu_core_get_i_cache(const mmu_core_handle* ch)
{
    return (ch->flags >> COREH_ICACHE_SHIFT) & 1ULL;
}

static inline bool mmu_core_get_align_trap(const mmu_core_handle* ch)
{
    return (ch->flags >> COREH_ALIGN_TRAP_SHIFT) & 1ULL;
}

static inline bool mmu_core_get_lo_enabled(const mmu_core_handle* ch)
{
    return (ch->flags >> COREH_LO_ENABLE_SHIFT) & 1ULL;
}

static inline bool mmu_core_get_hi_enabled(const mmu_core_handle* ch)
{
    return (ch->flags >> COREH_HI_ENABLE_SHIFT) & 1ULL;
}

static inline uint8_t mmu_core_get_lo_va_bits(const mmu_core_handle* ch)
{
    return (ch->flags >> COREH_LO_BITS_SHIFT) & COREH_VA_BITS_MASK;
}

static inline uint8_t mmu_core_get_hi_va_bits(const mmu_core_handle* ch)
{
    return (ch->flags >> COREH_HI_BITS_SHIFT) & COREH_VA_BITS_MASK;
}

static inline mmu_granularity
mmu_core_get_lo_granularity(const mmu_core_handle* ch)
{
    uint64_t tg0 =
        (ch->flags >> COREH_LO_GRANULARITY_SHIFT) & COREH_GRANULARITY_MASK;

    switch (tg0) {
        case TG0_4KB:
            return MMU_GRANULARITY_4KB;

        case TG0_16KB:
            return MMU_GRANULARITY_16KB;

        case TG0_64KB:
            return MMU_GRANULARITY_64KB;
    }

    PANIC();
}

static inline mmu_granularity
mmu_core_get_hi_granularity(const mmu_core_handle* ch)
{
    uint64_t tg1 =
        (ch->flags >> COREH_HI_GRANULARITY_SHIFT) & COREH_GRANULARITY_MASK;

    switch (tg1) {
        case TG1_4KB:
            return MMU_GRANULARITY_4KB;

        case TG1_16KB:
            return MMU_GRANULARITY_16KB;

        case TG1_64KB:
            return MMU_GRANULARITY_64KB;
    }

    PANIC();
}

static inline uint32_t mmu_core_get_coreid(const mmu_core_handle* ch)
{
    return ch->mpidr_aff;
}

bool mmu_core_set_mapping(mmu_core_handle* ch, mmu_mapping* t);

bool mmu_core_set_d_cache(mmu_core_handle* ch, bool v);
bool mmu_core_set_i_cache(mmu_core_handle* ch, bool v);
bool mmu_core_set_align_trap(mmu_core_handle* ch, bool v);
bool mmu_core_set_lo_enabled(mmu_core_handle* ch, bool v);
bool mmu_core_set_hi_enabled(mmu_core_handle* ch, bool v);
bool mmu_core_set_lo_va_bits(mmu_core_handle* ch, uint8_t bits);
bool mmu_core_set_hi_va_bits(mmu_core_handle* ch, uint8_t bits);
bool mmu_core_set_lo_granularity(mmu_core_handle* ch, mmu_granularity g);
bool mmu_core_set_hi_granularity(mmu_core_handle* ch, mmu_granularity g);


static inline mmu_mapping mmu_mapping_new(
    mmu_tbl_rng rng,
    mmu_granularity g,
    size_t va_addr_bits,
    uintptr_t physmap_offset,
    mmu_allocator allocator,
    mmu_allocator_free allocator_free)
{
    ASSERT(allocator);

    void* tbl = allocator(g);

    ASSERT(tbl && (vuintptr_t)tbl % g == 0);

    return (mmu_mapping) {
        .rng_ = rng,
        .g_ = g,
        .va_addr_bits_ = va_addr_bits,
        .physmap_offset_ = physmap_offset,
        .tbl_ = tbl,
        .allocator_ = allocator,
        .allocator_free_ = allocator_free,
    };
}

typedef enum {
    MMU_ACTIVATE_OK,
    MMU_ACTIVATE_NOT_THE_SAME_CORE, // a core can only activate its own mmu

    MMU_ACTIVATE_LO_NOT_ENABLED, // LO must be enabled, as if the mmu is
                                 // deactivated, it is running in physical
                                 // addresses, so the next pc instruction will
                                 // be in the LO table. HI does not need to be
                                 // enabled.

    MMU_ACTIVATE_IS_ALREADY_ACTIVE,
    MMU_ACTIVATE_LO_MAPPING_NOT_VALID,

    MMU_ACTIVATE_INVALID_LO_BITS,
    MMU_ACTIVATE_INVALID_HI_BITS,
    MMU_ACTIVATE_INVALID_LO_AND_HI_BITS,
} mmu_activate_result;

mmu_activate_result mmu_core_activate(mmu_core_handle* ch);

typedef enum {
    MMU_DEACTIVATE_OK,
    MMU_DEACTIVATE_NOT_THE_SAME_CORE,
    MMU_DEACTIVATE_IS_ALREADY_DEACTIVATED,
    MMU_DEACTIVATE_PC_IS_IN_HI_RNG,
} mmu_deactivate_result;

mmu_deactivate_result mmu_core_deactivate(mmu_core_handle* ch);


#ifndef __MMU_INTERNAL
#    undef COREH_DCACHE_SHIFT
#    undef COREH_ICACHE_SHIFT
#    undef COREH_ALIGN_TRAP_SHIFT
#    undef COREH_LO_ENABLE_SHIFT
#    undef COREH_HI_ENABLE_SHIFT
#    undef COREH_LO_BITS_SHIFT
#    undef COREH_HI_BITS_SHIFT
#    undef COREH_COREID_SHIFT
#    undef COREH_LO_GRANULARITY_SHIFT
#    undef COREH_HI_GRANULARITY_SHIFT
#    undef COREH_VA_BITS_WIDTH
#    undef COREH_VA_BITS_MASK
#    undef COREH_COREID_WIDTH
#    undef COREH_COREID_MASK
#    undef COREH_GRANULARITY_WIDTH
#    undef COREH_GRANULARITY_MASK
#    undef COREH_BIT_MASK
#    undef TG0_4KB
#    undef TG0_16KB
#    undef TG0_64KB
#    undef TG1_4KBs
#    undef TG1_16KB
#    undef TG1_64KB
#    undef NULL_MAPPING_TBL
#    undef MMU_APPLY_CHANGES
#endif // ifndef __MMU_INTERNAL


/*
 *  MMU main functions
 */
typedef enum {
    MMU_AP_EL0_NONE_EL1_RW = 0b00,
    MMU_AP_EL0_RW_EL1_RW = 0b01,
    MMU_AP_EL0_NONE_EL1_RO = 0b10,
    MMU_AP_EL0_RO_EL1_RO = 0b11,
} mmu_access_permission;

typedef enum {
    MMU_SH_NON_SHAREABLE = 0b00,
    MMU_SH_RESERVED = 0b01,
    MMU_SH_OUTER_SHAREABLE = 0b10,
    MMU_SH_INNER_SHAREABLE = 0b11,
} mmu_shareability;


typedef struct {
    _Alignas(4) uint8_t attr_index;
    mmu_access_permission ap;
    uint8_t shareability;
    _Alignas(4) bool non_secure;
    _Alignas(4) bool access_flag;
    _Alignas(4) bool pxn;
    _Alignas(4) bool uxn;
    _Alignas(4) uint8_t sw;
} mmu_pg_cfg;

typedef struct {
    size_t iters;
    size_t alocated_tbls;
    size_t freed_tbls;
} mmu_op_info;

typedef enum {
    MMU_MAP_OK,
    MMU_MAP_NULL_MAPPING,
    MMU_MAP_MAP_NOT_IN_RANGE,
} mmu_map_result;

static inline mmu_op_info mmu_op_info_new()
{
    return (mmu_op_info) {0};
}

static inline mmu_pg_cfg mmu_pg_cfg_new(
    uint8_t attr_index,
    mmu_access_permission ap,
    uint8_t shareability,
    bool non_secure,
    bool access_flag,
    bool pxn,
    bool uxn,
    uint8_t sw)
{
    return (mmu_pg_cfg) {
        .attr_index = attr_index,
        .ap = ap,
        .shareability = shareability,
        .non_secure = non_secure,
        .access_flag = access_flag,
        .pxn = pxn,
        .uxn = uxn,
        .sw = sw,
    };
}

mmu_map_result mmu_map(
    const mmu_mapping* m,
    vuintptr_t va,
    puintptr_t pa,
    size_t size,
    mmu_pg_cfg cfg,
    mmu_op_info* info);

typedef enum {
    MMU_UNMAP_ERR = 0,
    MMU_UNMAP_OK = 1,
} mmu_unmap_result;

mmu_unmap_result
mmu_unmap(const mmu_mapping* m, vuintptr_t va, size_t size, mmu_op_info* info);

bool mmu_is_active();
