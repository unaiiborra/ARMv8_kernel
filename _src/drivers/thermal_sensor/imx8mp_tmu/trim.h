#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

#define TMU_TRIM_OFFSET 0x3CUL

#define TRIM_VALUE_STRUCT_NAME TmuTrimValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TRIM_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(TMU, TRIM, TRIM_VALUE_STRUCT_NAME, TMU_TRIM_OFFSET);
MMIO_DECLARE_REG32_WRITER(TMU, TRIM, TRIM_VALUE_STRUCT_NAME, TMU_TRIM_OFFSET);

// Helper
#define TRIM_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(              \
        TRIM,                                  \
        bf_name,                               \
        TRIM_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define BGR_SHIFT 28
#define BGR_MASK  (0xFu << BGR_SHIFT)
TRIM_DECLARE_BIT_FIELD_FNS(BGR, uint8_t);

#define BJT_CUR_SHIFT 20
#define BJT_CUR_MASK  (0xFu << BJT_CUR_SHIFT)
TRIM_DECLARE_BIT_FIELD_FNS(BJT_CUR, uint8_t);

#define VLSB_SHIFT 12
#define VLSB_MASK  (0xFu << VLSB_SHIFT)
TRIM_DECLARE_BIT_FIELD_FNS(VLSB, uint8_t);

#define EN_CH_SHIFT 7
#define EN_CH_MASK  (0b1u << EN_CH_SHIFT)
TRIM_DECLARE_BIT_FIELD_FNS(EN_CH, bool);

#define EN_VREFT_TRIM_SHIFT 6
#define EN_VREFT_TRIM_MASK  (0b1u << EN_VREFT_TRIM_SHIFT)
TRIM_DECLARE_BIT_FIELD_FNS(EN_VREFT_TRIM, bool);

#define EN_VBE_TRIM_SHIFT 5
#define EN_VBE_TRIM_MASK  (0b1u << EN_VBE_TRIM_SHIFT)
TRIM_DECLARE_BIT_FIELD_FNS(EN_VBE_TRIM, bool);

#define VBE_FLAG_SHIFT 1
#define VBE_FLAG_MASK  (0b1u << VBE_FLAG_SHIFT)
TRIM_DECLARE_BIT_FIELD_FNS(VBE_FLAG, bool);

#define VREFT_FLAG_SHIFT 0
#define VREFT_FLAG_MASK  (0b1u << VREFT_FLAG_SHIFT)
TRIM_DECLARE_BIT_FIELD_FNS(VREFT_FLAG, bool);
