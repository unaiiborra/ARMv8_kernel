#pragma once


#include <kernel/hardware.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>

static inline uint64_t get_cpuid()
{
    uintptr_t v;
    asm volatile("mrs %0, sp_el0" : "=r"(v) : : "memory");

    uint64_t cpuid = v & 0xFF;

    DEBUG_ASSERT(cpuid < NUM_CORES);

    return cpuid;
};


bool wake_core(uint64_t core_id, uintptr_t entry_addr, uint64_t context);
