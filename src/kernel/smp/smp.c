#include <arm/mmu.h>
#include <arm/smccc/psci.h>
#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/panic.h>
#include <kernel/smp.h>
#include <kernel/time.h>
#include <lib/unit/mem.h>
#include <stdatomic.h>
#include <stddef.h>

#include "kernel/init.h"

#define PANIC_ENUM(enum_v) PANIC(#enum_v)

extern void _start(void);

static volatile atomic_bool initialized_cpus[NUM_CPUS];

[[gnu::noinline]] static void smp_gdb_thread_detect()
{
    asm volatile("nop");
}

void smp_init()
{
    for (cpuid_t cpu = 0; cpu < NUM_CPUS; cpu++) {
        if (cpu == get_cpuid())
            continue;

        psci_return_code code = psci_cpu_on(
            (arm_cpu_affinity) {.aff0 = cpu},
            as_kpa((void*)_start),
            cpu);

        switch (code) {
            case PSCI_SUCCESS:
                break;
            case PSCI_INVALID_PARAMETERS:
                PANIC_ENUM(PSCI_INVALID_PARAMETERS);
            case PSCI_INVALID_ADDRESS:
                PANIC_ENUM(PSCI_INVALID_ADDRESS);
            case PSCI_ALREADY_ON:
                PANIC_ENUM(PSCI_ALREADY_ON);
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

    ksleep(5e6);

    atomic_store(&initialized_cpus[get_cpuid()], true);

    while (true) {
        cpuid_t cpu = 0;
        for (; cpu < NUM_CPUS; cpu++) {
            if (!atomic_load(&initialized_cpus[cpu]))
                break;
        }

        if (cpu >= NUM_CPUS)
            break;
    }

    smp_gdb_thread_detect();
}

void setup_secondary_core_el2()
{
    bool result = mmu_core_handle_new(
        as_kpa(mm_mmu_core_handler_get_self()),
        as_kpa(MM_MMU_UNMAPPED_LO),
        as_kpa(MM_MMU_KERNEL_MAPPING),
        true,
        true,
        true,
        true,
        false);
    ASSERT(result);

    mmu_activate_result cres = mmu_core_activate(
        as_kpa(mm_mmu_core_handler_get_self()));
    ASSERT(cres == MMU_ACTIVATE_OK);
}

void setup_secondary_core_el1()
{
    extern noreturn void kernel_entry();
    extern void          _reset_stack_and_branch(
        void* stack_bottom,
        void* return_address);

    void* stack_bottom = ((char*)kmalloc(MEM_MiB(4))) + MEM_MiB(4);

    kernel_cpu_local_init();
    arm_exceptions_enable_all();

    atomic_store(&initialized_cpus[get_cpuid()], true);
    _reset_stack_and_branch(stack_bottom, kernel_entry);
}
