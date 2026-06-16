#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TIDR_OFFSET 0xCUL

#define TIDR_VALUE_STRUCT_NAME TmuTidrValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TIDR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(TMU, TIDR, TIDR_VALUE_STRUCT_NAME, TMU_TIDR_OFFSET);

MMIO_DECLARE_REG32_WRITER(TMU, TIDR, TIDR_VALUE_STRUCT_NAME, TMU_TIDR_OFFSET);

// Helper
#define TIDR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(              \
        TIDR,                                  \
        bf_name,                               \
        TIDR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    TMU_DECLARE_BIT_FIELD_SETTER(              \
        TIDR,                                  \
        bf_name,                               \
        TIDR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define ITTE1_SHIFT 31
#define ITTE1_MASK  (0b1u << ITTE1_SHIFT)
TIDR_DECLARE_BIT_FIELD_FNS(ITTE1, bool);

#define ATTE1_SHIFT 30
#define ATTE1_MASK  (0b1u << ATTE1_SHIFT)
TIDR_DECLARE_BIT_FIELD_FNS(ATTE1, bool);

#define ATCTE1_SHIFT 29
#define ATCTE1_MASK  (0b1u << ATCTE1_SHIFT)
TIDR_DECLARE_BIT_FIELD_FNS(ATCTE1, bool);

#define ITTE0_SHIFT 27
#define ITTE0_MASK  (0b1u << ITTE0_SHIFT)
TIDR_DECLARE_BIT_FIELD_FNS(ITTE0, bool);

#define ATTE0_SHIFT 26
#define ATTE0_MASK  (0b1u << ATTE0_SHIFT)
TIDR_DECLARE_BIT_FIELD_FNS(ATTE0, bool);

#define ATCTE0_SHIFT 25
#define ATCTE0_MASK  (0b1u << ATCTE0_SHIFT)
TIDR_DECLARE_BIT_FIELD_FNS(ATCTE0, bool);
