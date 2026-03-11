#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TIER_OFFSET 0x8UL

#define TIER_VALUE_STRUCT_NAME TmuTierValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TIER_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(TMU, TIER, TIER_VALUE_STRUCT_NAME, TMU_TIER_OFFSET);

MMIO_DECLARE_REG32_WRITER(TMU, TIER, TIER_VALUE_STRUCT_NAME, TMU_TIER_OFFSET);

// Helper
#define TIER_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(              \
        TIER,                                  \
        bf_name,                               \
        TIER_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    TMU_DECLARE_BIT_FIELD_SETTER(              \
        TIER,                                  \
        bf_name,                               \
        TIER_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define ITTEIE1_SHIFT 31
#define ITTEIE1_MASK (0b1u << ITTEIE1_SHIFT)
TIER_DECLARE_BIT_FIELD_FNS(ITTEIE1, bool);

#define ATTEIE1_SHIFT 30
#define ATTEIE1_MASK (0b1u << ATTEIE1_SHIFT)
TIER_DECLARE_BIT_FIELD_FNS(ATTEIE1, bool);

#define ATCTEIE1_SHIFT 29
#define ATCTEIE1_MASK (0b1u << ATCTEIE1_SHIFT)
TIER_DECLARE_BIT_FIELD_FNS(ATCTEIE1, bool);

#define ITTEIE0_SHIFT 27
#define ITTEIE0_MASK (0b1u << ITTEIE0_SHIFT)
TIER_DECLARE_BIT_FIELD_FNS(ITTEIE0, bool);

#define ATTEIE0_SHIFT 26
#define ATTEIE0_MASK (0b1u << ATTEIE0_SHIFT)
TIER_DECLARE_BIT_FIELD_FNS(ATTEIE0, bool);

#define ATCTEIE0_SHIFT 25
#define ATCTEIE0_MASK (0b1u << ATCTEIE0_SHIFT)
TIER_DECLARE_BIT_FIELD_FNS(ATCTEIE0, bool);
