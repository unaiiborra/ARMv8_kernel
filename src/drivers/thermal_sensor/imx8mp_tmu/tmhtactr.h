#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TMHTACTR_OFFSET 0x18UL

#define TMHTACTR_VALUE_STRUCT_NAME TmuTmhtactrValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TMHTACTR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(
    TMU,
    TMHTACTR,
    TMHTACTR_VALUE_STRUCT_NAME,
    TMU_TMHTACTR_OFFSET);

MMIO_DECLARE_REG32_WRITER(
    TMU,
    TMHTACTR,
    TMHTACTR_VALUE_STRUCT_NAME,
    TMU_TMHTACTR_OFFSET);

// Helper
#define TMHTACTR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(                  \
        TMHTACTR,                                  \
        bf_name,                                   \
        TMHTACTR_VALUE_STRUCT_NAME,                \
        T,                                         \
        bf_name##_SHIFT,                           \
        bf_name##_MASK);                           \
    TMU_DECLARE_BIT_FIELD_SETTER(                  \
        TMHTACTR,                                  \
        bf_name,                                   \
        TMHTACTR_VALUE_STRUCT_NAME,                \
        T,                                         \
        bf_name##_SHIFT,                           \
        bf_name##_MASK);

#define EN1_SHIFT 31
#define EN1_MASK  (0b1u << EN1_SHIFT)
TMHTACTR_DECLARE_BIT_FIELD_FNS(EN1, bool);

#define EN0_SHIFT 30
#define EN0_MASK  (0b1u << EN0_SHIFT)
TMHTACTR_DECLARE_BIT_FIELD_FNS(EN0, bool);

#define TEMP1_SHIFT 16
#define TEMP1_MASK  (0xFFu << TEMP1_SHIFT)
TMHTACTR_DECLARE_BIT_FIELD_FNS(TEMP1, int8_t);

#define TEMP0_SHIFT 0
#define TEMP0_MASK  (0xFFu << TEMP0_SHIFT)
TMHTACTR_DECLARE_BIT_FIELD_FNS(TEMP0, int8_t);
