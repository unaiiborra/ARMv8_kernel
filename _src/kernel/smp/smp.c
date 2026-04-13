#include <arm/smccc/psci.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/smp.h>
#include <stddef.h>

#include "arm/mmu.h"
#include "kernel/io/stdio.h"
#include "kernel/mm/mmu.h"


extern void _smp_wakeup_entry(size_t context_id);

#define PANIC_ENUM(enum_v) PANIC(#enum_v)


void smp_init()
{
    cpuid_t self = get_cpuid();

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
}

static void smp_cfg_end();
void        smp_cpu_cfg(size_t context_id)
{
    (void)context_id;
    // the entry happens with the mmu disabled
    mmu_core_handle* ch = mm_mmu_core_handler_get_self();

    bool res = mmu_core_handle_new(
        ch,
        as_kpa(MM_MMU_IDENTITY_LO_MAPPING),
        as_kpa(MM_MMU_KERNEL_MAPPING),
        true,
        true,
        true,
        true,
        true);
    ASSERT(res);

    mmu_core_activate(as_kpa(ch));

    mm_reloc(as_kva((void*)smp_cfg_end));
}


extern void kernel_entry();
static void smp_cfg_end()
{
    mmu_core_set_mapping(mm_mmu_core_handler_get_self(), MM_MMU_UNMAPPED_LO);
    mmu_core_set_mapping(mm_mmu_core_handler_get_self(), MM_MMU_KERNEL_MAPPING);

    kernel_entry();
}