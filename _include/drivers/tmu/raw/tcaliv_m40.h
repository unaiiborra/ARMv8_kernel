#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

#define TMU_TCALIV_M40_OFFSET 0x38UL

#define TCALIV_M40_VALUE_STRUCT_NAME TmuTcalivM40Value

MMIO_DECLARE_REG32_VALUE_STRUCT(TCALIV_M40_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(
    TMU,
    TCALIV_M40,
    TCALIV_M40_VALUE_STRUCT_NAME,
    TMU_TCALIV_M40_OFFSET);
MMIO_DECLARE_REG32_WRITER(
    TMU,
    TCALIV_M40,
    TCALIV_M40_VALUE_STRUCT_NAME,
    TMU_TCALIV_M40_OFFSET);

// Helper
#define TCALIV_M40_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(                    \
        TCALIV_M40,                                  \
        bf_name,                                     \
        TCALIV_M40_VALUE_STRUCT_NAME,                \
        T,                                           \
        bf_name##_SHIFT,                             \
        bf_name##_MASK);

#define SNSR_M40C_1_SHIFT 16
#define SNSR_M40C_1_MASK  (0x0FFFu << SNSR_M40C_1_SHIFT)
TCALIV_M40_DECLARE_BIT_FIELD_FNS(SNSR_M40C_1, uint16_t);

#define SNSR_M40C_0_SHIFT 0
#define SNSR_M40C_0_MASK  (0x0FFFu << SNSR_M40C_0_SHIFT)
TCALIV_M40_DECLARE_BIT_FIELD_FNS(SNSR_M40C_0, uint16_t);
