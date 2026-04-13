#pragma once

#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>


constexpr size_t NUM_CPUS = NUM_CORES;

typedef uint32_t cpuid_t;


void smp_init();


static inline cpuid_t get_cpuid()
{
    // TODO: allow for other affinities
    cpuid_t cpuid = sysreg_read(mpidr_el1) & 0xFF;

    DEBUG_ASSERT(cpuid < NUM_CPUS);

    return cpuid;
}
