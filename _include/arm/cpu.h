#pragma once

#include <stddef.h>
#include <stdint.h>

#include "arm/sysregs/sysregs.h"

typedef struct {
    _Alignas(4) uint8_t aff3;
    uint8_t aff2;
    uint8_t aff1;
    uint8_t aff0;
} arm_cpu_affinity;


static inline uint32_t arm_cpu_affinity_as_u32(arm_cpu_affinity aff)
{
    return ((uint32_t)aff.aff3 << 24) | ((uint32_t)aff.aff2 << 16) |
           ((uint32_t)aff.aff1 << 8) | ((uint32_t)aff.aff0 << 0);
}

static inline arm_cpu_affinity arm_u32_as_cpu_affinity(uint32_t aff)
{
    arm_cpu_affinity out;

    out.aff0 = (aff >> 0) & 0xFF;
    out.aff1 = (aff >> 8) & 0xFF;
    out.aff2 = (aff >> 16) & 0xFF;
    out.aff3 = (aff >> 24) & 0xFF;

    return out;
}


static inline arm_cpu_affinity arm_get_cpu_affinity()
{
    uint64_t mpidr = sysreg_read(mpidr_el1);

    arm_cpu_affinity aff;
    aff.aff0 = (mpidr >> 0) & 0xFF;
    aff.aff1 = (mpidr >> 8) & 0xFF;
    aff.aff2 = (mpidr >> 16) & 0xFF;
    aff.aff3 = (mpidr >> 32) & 0xFF;

    return aff;
}

static inline uint32_t arm_get_cpu_affinity_as_u32()
{
    arm_cpu_affinity aff = arm_get_cpu_affinity();

    return arm_cpu_affinity_as_u32(aff);
}




extern uint64_t _ARM_get_sp();
