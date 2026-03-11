#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TRITSR_OFFSET 0x20UL

#define TRITSR_VALUE_STRUCT_NAME TmuTritsrValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TRITSR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(
    TMU,
    TRITSR,
    TRITSR_VALUE_STRUCT_NAME,
    TMU_TRITSR_OFFSET);

// Helper
#define TRITSR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(                \
        TRITSR,                                  \
        bf_name,                                 \
        TRITSR_VALUE_STRUCT_NAME,                \
        T,                                       \
        bf_name##_SHIFT,                         \
        bf_name##_MASK);

#define V1_SHIFT 31
#define V1_MASK (0b1u << V1_SHIFT)
TRITSR_DECLARE_BIT_FIELD_FNS(V1, bool);

#define V0_SHIFT 30
#define V0_MASK (0b1u << V0_SHIFT)
TRITSR_DECLARE_BIT_FIELD_FNS(V0, bool);

/* Bits 29–24 reservados */

#define TEMP1_SHIFT 16
#define TEMP1_MASK (0xFFu << TEMP1_SHIFT)
TRITSR_DECLARE_BIT_FIELD_FNS(TEMP1, int8_t);

/* Bits 15–8 reservados */

#define TEMP0_SHIFT 0
#define TEMP0_MASK (0xFFu << TEMP0_SHIFT)
TRITSR_DECLARE_BIT_FIELD_FNS(TEMP0, int8_t);
