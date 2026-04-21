#include <arm/smccc/psci.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/smp.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/mmu.h"
#include "kernel/io/stdio.h"
#include "kernel/mm/mmu.h"



extern void _smp_wakeup_entry(size_t context_id);
static void smp_cfg_end();

#define PANIC_ENUM(enum_v) PANIC(#enum_v)

#ifdef GDB
// used as a breakpoint for forcing new threads detection
[[gnu::noinline]] void smp_gdb_thread_detect()
{
    asm volatile("nop");
}

volatile uint64_t smp_gdb_barrier_detect[NUM_CPUS];
volatile uint64_t smp_gdb_barrier_hang[NUM_CPUS];

#else
#    define gdb_thread_detect()
#endif

void smp_init()
{
    cpuid_t self = get_cpuid();

#ifdef GDB
    for (cpuid_t i = 0; i < NUM_CPUS; i++) {
        smp_gdb_barrier_detect[i] = 0;
        smp_gdb_barrier_hang[i]   = 1;
    }

    smp_gdb_barrier_detect[self] = 1;
    smp_gdb_barrier_hang[self]   = 0xffffffffffffffff;
#endif

    for (size_t i = 0; i < NUM_CPUS; i++) {
        if (i == self)
            continue;

        psci_return_code code = psci_cpu_on(
            (arm_cpu_affinity) {.aff0 = i},
            as_kpa((void*)_smp_wakeup_entry),
            i);

        switch (code) {
            case PSCI_SUCCESS:
                break;
            case PSCI_INVALID_PARAMETERS:
                PANIC_ENUM(PSCI_INVALID_PARAMETERS);
            case PSCI_INVALID_ADDRESS:
                PANIC_ENUM(PSCI_INVALID_ADDRESS);
            case PSCI_ALREADY_ON:
                dbg_printf(
                    DEBUG_TRACE,
                    "smp_init: attempted to wake core %d but it was "
                    "already "
                    "awake",
                    i);
                break;
            case PSCI_ON_PENDING:
                PANIC_ENUM(PSCI_ON_PENDING);
            case PSCI_INTERNAL_FAILURE:
                PANIC_ENUM(PSCI_INTERNAL_FAILURE);
            case PSCI_DENIED:
                PANIC_ENUM(PSCI_DENIED);
            default:
                PANIC();
        }
    }

#ifdef GDB
retry:
    for (size_t i = 0; i < NUM_CPUS; i++)
        if (smp_gdb_barrier_detect[i] != 1)
            goto retry;

    smp_gdb_thread_detect();
#endif
}


void smp_cpu_cfg(size_t context_id)
{
    (void)context_id;

    // the entry happens with the mmu disabled
    mmu_core_handle* ch = mm_mmu_core_handler_get_self();

    bool res = mmu_core_handle_new(
        as_kpa(ch),
        as_kpa(MM_MMU_IDENTITY_LO_MAPPING),
        as_kpa(MM_MMU_KERNEL_MAPPING),
        true,
        true,
        true,
        true,
        false);
    ASSERT(res);

    mmu_core_activate(as_kpa(ch));
    mm_reloc(as_kva((void*)smp_cfg_end));
}


extern void kernel_entry();
static void smp_cfg_end()
{
#ifdef GDB
    cpuid_t cpuid                 = get_cpuid();
    smp_gdb_barrier_detect[cpuid] = 1;

    while (smp_gdb_barrier_hang[cpuid]) {
        asm volatile("wfi");
    }
#endif

    mmu_core_set_mapping(mm_mmu_core_handler_get_self(), MM_MMU_UNMAPPED_LO);
    mmu_core_set_mapping(mm_mmu_core_handler_get_self(), MM_MMU_KERNEL_MAPPING);

    kernel_entry();
}