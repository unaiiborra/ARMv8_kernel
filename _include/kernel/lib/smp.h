#pragma once


#include <kernel/hardware.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/sysregs/sysregs.h"


typedef uint32_t cpuid_t;


static inline cpuid_t get_cpuid()
{
    cpuid_t cpuid = sysreg_read(mpidr_el1) & 0xFF;

    DEBUG_ASSERT(cpuid < NUM_CORES);

    return cpuid;
};


bool wake_core(uint64_t core_id, uintptr_t entry_addr, uint64_t context);
