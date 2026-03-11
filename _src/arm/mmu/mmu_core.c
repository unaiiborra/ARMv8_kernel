#define __MMU_INTERNAL

#include <arm/cpu.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <lib/stdmacros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mmu_tbl_ctrl.h"
#include "mmu_types.h"
#include "regs/mmu_sysregs.h"


const mmu_mapping MMU_NULL_MAPPING = {
    .tbl_ = NULL_MAPPING_TBL,
};


static inline bool mmu_on(uint64_t sctlr)
{
    return sctlr & 1ULL;
}

static inline bool set_coreid(mmu_core_handle* ch)
{
    uint64_t MPIDR_EL1;

    asm volatile("mrs %0, mpidr_el1" : "=r"(MPIDR_EL1) : : "memory");

    uint32_t mpidr_aff =
        ((MPIDR_EL1 >> 0) & 0xFF) | ((MPIDR_EL1 >> 8) & 0xFF) << 8 |
        ((MPIDR_EL1 >> 16) & 0xFF) << 16 | ((MPIDR_EL1 >> 32) & 0xFF) << 24;

    ch->mpidr_aff = mpidr_aff;

    return true;
}

static inline bool eq_caller_coreid(mmu_core_handle* ch)
{
    uint64_t MPIDR_EL1;

    asm volatile("mrs %0, mpidr_el1" : "=r"(MPIDR_EL1) : : "memory");

    uint32_t mpidr_aff =
        ((MPIDR_EL1 >> 0) & 0xFF) | ((MPIDR_EL1 >> 8) & 0xFF) << 8 |
        ((MPIDR_EL1 >> 16) & 0xFF) << 16 | ((MPIDR_EL1 >> 32) & 0xFF) << 24;

    return ch->mpidr_aff == mpidr_aff;
}

bool mmu_core_set_mapping(mmu_core_handle* ch, mmu_mapping* t)
{
    ASSERT(ch);

    uint64_t sctlr = _mmu_get_SCTLR_EL1();

    if (!mmu_mapping_is_valid(t))
        return false;

    if (mmu_on(sctlr)) {
        if (!eq_caller_coreid(ch))
            return false;

        switch (t->rng_) {
            case MMU_LO:
                ch->lo_mapping = t;
                _mmu_set_TTBR0_EL1(
                    (v_uintptr_t)ch->lo_mapping->tbl_ -
                    ch->lo_mapping->physmap_offset_);
                break;

            case MMU_HI:
                ch->hi_mapping = t;
                _mmu_set_TTBR1_EL1(
                    (v_uintptr_t)ch->hi_mapping->tbl_ -
                    ch->hi_mapping->physmap_offset_);
                break;

            default:
                PANIC();
        }

        MMU_APPLY_CHANGES();
    }
    else {
        switch (t->rng_) {
            case MMU_LO:
                ch->lo_mapping = t;
                break;

            case MMU_HI:
                ch->hi_mapping = t;
                break;

            default:
                PANIC();
        }
    }

    return true;
} /* mmu_core_set_mapping */

bool mmu_core_set_d_cache(mmu_core_handle* ch, bool v)
{
    ASSERT(ch);
    uint64_t sctlr = _mmu_get_SCTLR_EL1();

    if (mmu_on(sctlr))
        return false;

    ch->flags &= ~(COREH_BIT_MASK(COREH_DCACHE_SHIFT));
    ch->flags |= (uint64_t)v << COREH_DCACHE_SHIFT;

    return true;
}

bool mmu_core_set_i_cache(mmu_core_handle* ch, bool v)
{
    ASSERT(ch);
    uint64_t sctlr = _mmu_get_SCTLR_EL1();

    if (mmu_on(sctlr))
        return false;

    ch->flags &= ~(COREH_BIT_MASK(COREH_ICACHE_SHIFT));
    ch->flags |= (uint64_t)v << COREH_ICACHE_SHIFT;

    return true;
}

bool mmu_core_set_align_trap(mmu_core_handle* ch, bool v)
{
    ASSERT(ch);

    uint64_t sctlr = _mmu_get_SCTLR_EL1();
    if (mmu_on(sctlr))
        return false;

    ch->flags &= ~COREH_BIT_MASK(COREH_ALIGN_TRAP_SHIFT);
    ch->flags |= (uint64_t)v << COREH_ALIGN_TRAP_SHIFT;

    return true;
}

bool mmu_core_set_lo_enabled(mmu_core_handle* ch, bool v)
{
    ASSERT(ch);
    uint64_t sctlr = _mmu_get_SCTLR_EL1();
    if (mmu_on(sctlr))
        return false;

    ch->flags &= ~COREH_BIT_MASK(COREH_LO_ENABLE_SHIFT);
    ch->flags |= (uint64_t)v << COREH_LO_ENABLE_SHIFT;

    return true;
}

bool mmu_core_set_hi_enabled(mmu_core_handle* ch, bool v)
{
    ASSERT(ch);
    uint64_t sctlr = _mmu_get_SCTLR_EL1();
    if (mmu_on(sctlr))
        return false;

    ch->flags &= ~COREH_BIT_MASK(COREH_HI_ENABLE_SHIFT);
    ch->flags |= (uint64_t)v << COREH_HI_ENABLE_SHIFT;

    return true;
}

bool mmu_core_set_lo_va_bits(mmu_core_handle* ch, uint8_t bits)
{
    ASSERT(ch);
    uint64_t sctlr = _mmu_get_SCTLR_EL1();
    if (mmu_on(sctlr))
        return false;

    if (bits >= (1u << COREH_VA_BITS_WIDTH))
        return false;

    ch->flags &= ~((uint64_t)COREH_VA_BITS_MASK << COREH_LO_BITS_SHIFT);
    ch->flags |= (uint64_t)bits << COREH_LO_BITS_SHIFT;

    return true;
}

bool mmu_core_set_hi_va_bits(mmu_core_handle* ch, uint8_t bits)
{
    ASSERT(ch);
    uint64_t sctlr = _mmu_get_SCTLR_EL1();
    if (mmu_on(sctlr))
        return false;

    if (bits >= (1u << COREH_VA_BITS_WIDTH))
        return false;

    ch->flags &= ~((uint64_t)COREH_VA_BITS_MASK << COREH_HI_BITS_SHIFT);
    ch->flags |= (uint64_t)bits << COREH_HI_BITS_SHIFT;

    return true;
}

bool mmu_core_set_lo_granularity(mmu_core_handle* ch, mmu_granularity g)
{
    ASSERT(ch);
    uint64_t sctlr = _mmu_get_SCTLR_EL1();
    if (mmu_on(sctlr))
        return false;

    uint64_t val;
    switch (g) {
        case MMU_GRANULARITY_4KB:
            val = TG0_4KB;
            break;

        case MMU_GRANULARITY_16KB:
            val = TG0_16KB;
            break;

        case MMU_GRANULARITY_64KB:
            val = TG0_64KB;
            break;

        default:
            return false;
    }

    ch->flags &=
        ~((uint64_t)COREH_GRANULARITY_MASK << COREH_LO_GRANULARITY_SHIFT);
    ch->flags |= val << COREH_LO_GRANULARITY_SHIFT;

    return true;
} /* mmu_core_set_lo_granularity */

bool mmu_core_set_hi_granularity(mmu_core_handle* ch, mmu_granularity g)
{
    ASSERT(ch);
    uint64_t sctlr = _mmu_get_SCTLR_EL1();
    if (mmu_on(sctlr))
        return false;

    uint64_t val;
    switch (g) {
        case MMU_GRANULARITY_4KB:
            val = TG1_4KB;
            break;

        case MMU_GRANULARITY_16KB:
            val = TG1_16KB;
            break;

        case MMU_GRANULARITY_64KB:
            val = TG1_64KB;
            break;

        default:
            return false;
    }

    ch->flags &=
        ~((uint64_t)COREH_GRANULARITY_MASK << COREH_HI_GRANULARITY_SHIFT);
    ch->flags |= val << COREH_HI_GRANULARITY_SHIFT;

    return true;
} /* mmu_core_set_hi_granularity */

bool mmu_core_handle_new(
    mmu_core_handle* out,
    mmu_mapping* lo_mapping,
    mmu_mapping* hi_mapping,
    bool hi_enable,
    bool lo_enable,
    bool d_cache,
    bool i_cache,
    bool align_trap)
{
    ASSERT(lo_mapping && hi_mapping);

    if (lo_mapping->rng_ != MMU_LO)
        return false;

    if (hi_mapping->rng_ != MMU_HI)
        return false;

    if (!mmu_mapping_is_valid(lo_mapping) && lo_enable)
        return false;

    if (!mmu_mapping_is_valid(hi_mapping) && hi_enable)
        return false;

#define MMU_CORE_ASSERT_SET(fn) \
    if (!fn)                    \
    return false

    MMU_CORE_ASSERT_SET(mmu_core_set_mapping(out, lo_mapping));

    MMU_CORE_ASSERT_SET(mmu_core_set_mapping(out, hi_mapping));
    MMU_CORE_ASSERT_SET(mmu_core_set_d_cache(out, d_cache));
    MMU_CORE_ASSERT_SET(mmu_core_set_i_cache(out, i_cache));
    MMU_CORE_ASSERT_SET(mmu_core_set_align_trap(out, align_trap));
    MMU_CORE_ASSERT_SET(mmu_core_set_lo_enabled(out, lo_enable));
    MMU_CORE_ASSERT_SET(mmu_core_set_hi_enabled(out, hi_enable));
    MMU_CORE_ASSERT_SET(
        mmu_core_set_lo_va_bits(out, lo_mapping->va_addr_bits_));
    MMU_CORE_ASSERT_SET(
        mmu_core_set_hi_va_bits(out, hi_mapping->va_addr_bits_));
    MMU_CORE_ASSERT_SET(mmu_core_set_lo_granularity(out, lo_mapping->g_));
    MMU_CORE_ASSERT_SET(mmu_core_set_hi_granularity(out, hi_mapping->g_));

    MMU_CORE_ASSERT_SET(set_coreid(out));

#undef MMU_CORE_ASSERT_SET

    return true;
} /* mmu_core_handle_new */

void mmu_delete_mapping(mmu_mapping* m)
{
    free_tbl(m, mmu_mapping_get_tbl(m), MMU_TBL_LV0, NULL);

    *m = MMU_NULL_MAPPING;
}

mmu_activate_result mmu_core_activate(mmu_core_handle* ch)
{
    ASSERT(ch);

    if (!eq_caller_coreid(ch))
        return MMU_ACTIVATE_NOT_THE_SAME_CORE;

    if (!mmu_core_get_lo_enabled(ch))
        return MMU_ACTIVATE_LO_NOT_ENABLED;

    if (!mmu_mapping_is_valid(ch->lo_mapping))
        return MMU_ACTIVATE_LO_MAPPING_NOT_VALID;

    uint64_t sctlr = _mmu_get_SCTLR_EL1();
    if ((sctlr & 1ULL) == 1)
        return MMU_ACTIVATE_IS_ALREADY_ACTIVE;


    uint8_t lo_va_bits = mmu_core_get_lo_va_bits(ch);
    uint8_t hi_va_bits = mmu_core_get_hi_va_bits(ch);
    bool valid_lo_bits = lo_va_bits >= 39 && lo_va_bits <= 48;
    bool valid_hi_bits = hi_va_bits >= 39 && hi_va_bits <= 48;
    uint64_t tg0 =
        (ch->flags >> COREH_LO_GRANULARITY_SHIFT) & COREH_GRANULARITY_MASK;
    uint64_t tg1 =
        (ch->flags >> COREH_HI_GRANULARITY_SHIFT) & COREH_GRANULARITY_MASK;


    if (!valid_lo_bits && !valid_hi_bits)
        return MMU_ACTIVATE_INVALID_LO_AND_HI_BITS;
    if (!valid_lo_bits)
        return MMU_ACTIVATE_INVALID_LO_BITS;
    if (!valid_hi_bits)
        return MMU_ACTIVATE_INVALID_HI_BITS;


    /*
     * TODO: configurable mair
     * AttrIdx 0: Normal memory, WB WA
     * AttrIdx 1: Device-nGnRE
     * https://df.lth.se/~getz/ARM/SysReg/AArch64-mair_el1.html
     */
    uint64_t mair = (0xFFUL << 0) | (0x04UL << 8);
    uint64_t tcr = 0;


    /* vaddress size */
    tcr |= (uint64_t)(64 - lo_va_bits) << 0;  /* T0SZ */
    tcr |= (uint64_t)(64 - hi_va_bits) << 16; /* T1SZ */

    /* enable/disable TTBRs */
    tcr |= (uint64_t)(!mmu_core_get_lo_enabled(ch)) << 7;  /* EPD0 */
    tcr |= (uint64_t)(!mmu_core_get_hi_enabled(ch)) << 23; /* EPD1 */

    /* granularity */
    tcr |= tg0 << 14; /* TG0 */
    tcr |= tg1 << 30; /* TG1 */

    /* cacheability shareability TODO: allow the user to define it */
    tcr |= 0b11ULL << 12; /* SH0 = Inner Shareable */
    tcr |= 0b01ULL << 10; /* ORGN0 = WB WA */
    tcr |= 0b01ULL << 8;  /* IRGN0 = WB WA */

    tcr |= 0b11ULL << 28; /* SH1 = Inner Shareable */
    tcr |= 0b01ULL << 26; /* ORGN1 = WB WA */
    tcr |= 0b01ULL << 24; /* IRGN1 = WB WA */


    _mmu_set_TTBR0_EL1(
        (uintptr_t)ch->lo_mapping->tbl_ - ch->lo_mapping->physmap_offset_);
    if (mmu_core_get_hi_enabled(ch)) {
        _mmu_set_TTBR1_EL1(
            (uintptr_t)ch->hi_mapping->tbl_ - ch->hi_mapping->physmap_offset_);
    }


    /* paddress size */
    uint64_t id_aa64mmfr0 = _mmu_get_ID_AA64MMFR0_EL1();
    uint64_t pa_range =
        id_aa64mmfr0 & 0xFUL; /* [3:0] DDI0500J_cortex_a53_trm.pdf p.104 */

    uint64_t ips;
    switch (pa_range) {
        case 0b0000:
            ips = 0b000;
            break; /* 32 bits */

        case 0b0001:
            ips = 0b001;
            break; /* 36 bits */

        case 0b0010:
            ips = 0b010;
            break; /* 40 bits */

        case 0b0011:
            ips = 0b011;
            break; /* 42 bits */

        case 0b0100:
            ips = 0b100;
            break; /* 44 bits */

        case 0b0101:
            ips = 0b101;
            break; /* 48 bits */

        default:
            PANIC("Unsupported PA range");
    } /* switch */

    tcr |= (uint64_t)(ips & 0b111ULL) << 32; /* IPS */

    sctlr &= ~(1ULL << 1);
    sctlr &= ~(1ULL << 2);
    sctlr &= ~(1ULL << 12);

    sctlr |=
        ((uint64_t)mmu_core_get_i_cache(ch) << 12); /* I instruction cache */
    sctlr |= ((uint64_t)mmu_core_get_d_cache(ch) << 2); /* D data cache */
    sctlr |=
        ((uint64_t)mmu_core_get_align_trap(ch) << 1); /* A alignment trap */
    sctlr |= (1ULL << 0);                             /* M MMU enable */


    _mmu_set_MAIR_EL1(mair);
    _mmu_set_TCR_EL1(tcr);

    MMU_APPLY_CHANGES();

    _mmu_set_SCTLR_EL1(sctlr);

    MMU_APPLY_CHANGES();

    return MMU_ACTIVATE_OK;
} /* mmu_core_activate */

mmu_deactivate_result mmu_core_deactivate(mmu_core_handle* ch)
{
    uint64_t sctlr = _mmu_get_SCTLR_EL1();

    if (!eq_caller_coreid(ch))
        return MMU_DEACTIVATE_NOT_THE_SAME_CORE;

    if ((sctlr & 1ULL) == 0)
        return MMU_DEACTIVATE_IS_ALREADY_DEACTIVATED;

    uintptr_t pc;
    asm volatile("adrp %0, ." : "=r"(pc) : : "memory");

    if ((pc & (1ULL << 63)) != 0)
        return MMU_DEACTIVATE_PC_IS_IN_HI_RNG;

    _mmu_set_SCTLR_EL1(sctlr & ~1ULL);

    MMU_APPLY_CHANGES();

    return MMU_DEACTIVATE_OK;
} /* mmu_core_deactivate */
