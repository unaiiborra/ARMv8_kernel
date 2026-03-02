#pragma once

#include <kernel/process/process.h>


/// Allocates the user region according to the provided usr_va. It also
/// allocates a kernel region mapped to the same pa range used by the usr region
/// so the kernel has access to the user region without need to reconfigure
/// ttbr0 each time it requires access. Returns the kernel ptr to the allocated
/// user region. It does neither flush caches nor memzero the region.
void* process_malloc_usr_region(
    proc* usr_proc,
    v_uintptr usr_va,
    size_t pages,
    mmu_pg_cfg* usr_mmu_cfg);