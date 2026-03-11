#pragma once


#include <kernel/hardware.h>
#include <kernel/panic.h>
#include <lib/stdint.h>


static inline uint64 get_cpuid()
{
    uintptr v;
    asm volatile("mrs %0, sp_el0" : "=r"(v) : : "memory");

    uint64 cpuid = v & 0xFF;

    DEBUG_ASSERT(cpuid < NUM_CORES);

    return cpuid;
};


bool wake_core(uint64 core_id, uintptr entry_addr, uint64 context);
