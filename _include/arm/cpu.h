#pragma once

#include <stddef.h>
#include <stdint.h>
typedef struct {
    _Alignas(4) uint8_t aff3;
    uint8_t aff2;
    uint8_t aff1;
    uint8_t aff0;
} arm_cpu_affinity;

arm_cpu_affinity arm_get_cpu_affinity();

#define CPU_AFFINITY_FROM_U32(x)               \
    ((arm_cpu_affinity) {                      \
        .aff0 = (uint8_t)((x) & 0xFF),         \
        .aff1 = (uint8_t)(((x) >> 8) & 0xFF),  \
        .aff2 = (uint8_t)(((x) >> 16) & 0xFF), \
        .aff3 = (uint8_t)(((x) >> 24) & 0xFF), \
    })

#define U32_FROM_CPU_AFFINITY(x)                                         \
    (uint32_t)(((uint32_t)(x).aff3 << 24) | ((uint32_t)(x).aff2 << 16) | \
               ((uint32_t)(x).aff1 << 8) | ((uint32_t)(x).aff0))

extern uint64_t _ARM_get_sp();
