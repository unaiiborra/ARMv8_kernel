#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TSCR_OFFSET 0x1CUL

#define TSCR_VALUE_STRUCT_NAME TmuTscrValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TSCR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(TMU, TSCR, TSCR_VALUE_STRUCT_NAME, TMU_TSCR_OFFSET);

// Helper
#define TSCR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(              \
        TSCR,                                  \
        bf_name,                               \
        TSCR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define V1_SHIFT 31
#define V1_MASK (0b1u << V1_SHIFT)
TSCR_DECLARE_BIT_FIELD_FNS(V1, bool);

#define V0_SHIFT 30
#define V0_MASK (0b1u << V0_SHIFT)
TSCR_DECLARE_BIT_FIELD_FNS(V0, bool);

#define SNSR1_SHIFT 16
#define SNSR1_MASK (0x0FFFu << SNSR1_SHIFT)
TSCR_DECLARE_BIT_FIELD_FNS(SNSR1, uint16_t);

#define SNSR0_SHIFT 0
#define SNSR0_MASK (0x0FFFu << SNSR0_SHIFT)
TSCR_DECLARE_BIT_FIELD_FNS(SNSR0, uint16_t);
