#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TASR_OFFSET 0x28UL

#define TASR_VALUE_STRUCT_NAME TmuTasrValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TASR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(TMU, TASR, TASR_VALUE_STRUCT_NAME, TMU_TASR_OFFSET);
MMIO_DECLARE_REG32_WRITER(TMU, TASR, TASR_VALUE_STRUCT_NAME, TMU_TASR_OFFSET);

// Helper
#define TASR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(              \
        TASR,                                  \
        bf_name,                               \
        TASR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define BUF_SLOP_SEL_SHIFT 16
#define BUF_SLOP_SEL_MASK  (0xFu << BUF_SLOP_SEL_SHIFT)
TASR_DECLARE_BIT_FIELD_FNS(BUF_SLOP_SEL, uint8_t);

#define BUF_VERF_SEL_SHIFT 0
#define BUF_VERF_SEL_MASK  (0x3u << BUF_VERF_SEL_SHIFT)
TASR_DECLARE_BIT_FIELD_FNS(BUF_VERF_SEL, uint8_t);
