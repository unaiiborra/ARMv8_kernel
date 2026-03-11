#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TPS_OFFSET 0x4UL

#define TPS_VALUE_STRUCT_NAME TmuTpsValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TPS_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(TMU, TPS, TPS_VALUE_STRUCT_NAME, TMU_TPS_OFFSET);

MMIO_DECLARE_REG32_WRITER(TMU, TPS, TPS_VALUE_STRUCT_NAME, TMU_TPS_OFFSET);

// Helper
#define TPS_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(             \
        TPS,                                  \
        bf_name,                              \
        TPS_VALUE_STRUCT_NAME,                \
        T,                                    \
        bf_name##_SHIFT,                      \
        bf_name##_MASK);                      \
    TMU_DECLARE_BIT_FIELD_SETTER(             \
        TPS,                                  \
        bf_name,                              \
        TPS_VALUE_STRUCT_NAME,                \
        T,                                    \
        bf_name##_SHIFT,                      \
        bf_name##_MASK);

typedef enum {
    TMU_TPS_PROBE_SEL_MAIN_PROBE,
    TMU_TPS_PROBE_SEL_REMOTE_PROBE,
    TMU_TPS_PROBE_SEL_BOTH_PROBES,
} TMU_TPS_PROBE_SEL_VALUE;

#define PROBE_SEL_SHIFT 30
#define PROBE_SEL_MASK (0b11u << PROBE_SEL_SHIFT)
TPS_DECLARE_BIT_FIELD_FNS(PROBE_SEL, TMU_TPS_PROBE_SEL_VALUE);
