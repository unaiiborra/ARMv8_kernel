#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TCALIV0_OFFSET 0x30UL

#define TCALIV0_VALUE_STRUCT_NAME TmuTcaliv0Value

MMIO_DECLARE_REG32_VALUE_STRUCT(TCALIV0_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(
    TMU,
    TCALIV0,
    TCALIV0_VALUE_STRUCT_NAME,
    TMU_TCALIV0_OFFSET);
MMIO_DECLARE_REG32_WRITER(
    TMU,
    TCALIV0,
    TCALIV0_VALUE_STRUCT_NAME,
    TMU_TCALIV0_OFFSET);

// Helper
#define TCALIV0_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(                 \
        TCALIV0,                                  \
        bf_name,                                  \
        TCALIV0_VALUE_STRUCT_NAME,                \
        T,                                        \
        bf_name##_SHIFT,                          \
        bf_name##_MASK);

#define EN_SHIFT 31
#define EN_MASK (0b1u << EN_SHIFT)
TCALIV0_DECLARE_BIT_FIELD_FNS(EN, bool);

#define SNSR105C_SHIFT 16
#define SNSR105C_MASK (0x0FFFu << SNSR105C_SHIFT)
TCALIV0_DECLARE_BIT_FIELD_FNS(SNSR105C, uint16_t);

#define SNSR25C_SHIFT 0
#define SNSR25C_MASK (0x0FFFu << SNSR25C_SHIFT)
TCALIV0_DECLARE_BIT_FIELD_FNS(SNSR25C, uint16_t);
