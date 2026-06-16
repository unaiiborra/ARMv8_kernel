#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TER_OFFSET 0x0UL

#define TER_VALUE_STRUCT_NAME TmuTerValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TER_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(TMU, TER, TER_VALUE_STRUCT_NAME, TMU_TER_OFFSET);

MMIO_DECLARE_REG32_WRITER(TMU, TER, TER_VALUE_STRUCT_NAME, TMU_TER_OFFSET);

// Helper
#define TER_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(             \
        TER,                                  \
        bf_name,                              \
        TER_VALUE_STRUCT_NAME,                \
        T,                                    \
        bf_name##_SHIFT,                      \
        bf_name##_MASK);                      \
    TMU_DECLARE_BIT_FIELD_SETTER(             \
        TER,                                  \
        bf_name,                              \
        TER_VALUE_STRUCT_NAME,                \
        T,                                    \
        bf_name##_SHIFT,                      \
        bf_name##_MASK);

#define EN_SHIFT 31
#define EN_MASK  (0b1u << EN_SHIFT)
TER_DECLARE_BIT_FIELD_FNS(EN, bool);

#define ADC_PD_SHIFT 30
#define ADC_PD_MASK  (0b1U << ADC_PD_SHIFT)
TER_DECLARE_BIT_FIELD_FNS(ADC_PD, bool);

typedef enum {
    TER_ALPF_VALUE_1     = 0x0u,
    TER_ALPF_VALUE_0_5   = 0x1u,
    TER_ALPF_VALUE_0_25  = 0x2u,
    TER_ALPF_VALUE_0_125 = 0x3u,
} TER_ALPF_VALUE;

#define ALPF_SHIFT 0
#define ALPF_MASK  (0b11U << ALPF_SHIFT)
TER_DECLARE_BIT_FIELD_FNS(ALPF, TER_ALPF_VALUE)
