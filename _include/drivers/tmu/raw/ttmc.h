#pragma once

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "tmu_map.h"

// 5.4.4.1 - 682

#define TMU_TTMC_OFFSET 0x2CUL

#define TTMC_VALUE_STRUCT_NAME TmuTtmcValue

MMIO_DECLARE_REG32_VALUE_STRUCT(TTMC_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(TMU, TTMC, TTMC_VALUE_STRUCT_NAME, TMU_TTMC_OFFSET);
MMIO_DECLARE_REG32_WRITER(TMU, TTMC, TTMC_VALUE_STRUCT_NAME, TMU_TTMC_OFFSET);

// Helper
#define TTMC_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    TMU_DECLARE_BIT_FIELD_GETTER(              \
        TTMC,                                  \
        bf_name,                               \
        TTMC_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define TMUX_SHIFT 0
#define TMUX_MASK 0x7
TTMC_DECLARE_BIT_FIELD_FNS(TMUX, uint8_t);
