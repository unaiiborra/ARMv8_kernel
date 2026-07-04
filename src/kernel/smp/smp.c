#include <arm/smccc/psci.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/smp.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/exceptions/exceptions.h"
#include "arm/mmu.h"
#include "arm/sysregs/sysregs.h"
#include "kernel/devices/driver_ops/irq_ctrl.h"
#include "kernel/init.h"
#include "kernel/io/stdio.h"
#include "kernel/mm/mmu.h"
#include "kernel/time.h"
#include "lib/stdattribute.h"


extern void _reset_stack_and_branch(void* stack_bottom, void* return_address);
extern void _smp_wakeup_entry(size_t context_id);
static void smp_cfg_end();

#define PANIC_ENUM(enum_v) PANIC(#enum_v)

#ifdef GDB
// used as a breakpoint for forcing new threads detection
volatile uint64_t smp_gdb_barrier_hang[NUM_CPUS];

[[gnu::noinline]] void smp_gdb_thread_detect()
{
    asm volatile("nop");
}


#else
#    define gdb_thread_detect()
#endif

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

#ifdef GDB
    ksleep(5e6); // 5ms
    smp_gdb_thread_detect();
#endif
}

safe_early static void* aloc_stack(size_t size)
{
    raw_kmalloc_cfg stack_cfg = {
        .fill_reserve = true,
        .assign_pa    = true,
        .kmap         = true,
        .device_mem   = false,
        .permanent    = true,
        .init_zeroed  = true,
    };

    const size_t STACK_SIZE = size;

    void* stack_top = raw_kmalloc(
        div_ceil(STACK_SIZE, PAGE_SIZE),
        "kernel stack region",
        &stack_cfg,
        NULL);

    void* stack_bottom = (char*)stack_top + STACK_SIZE;

    return stack_bottom;
}

safe_early static void init_devices()
{
    time_ctrl_init_cpu();

    arm_exceptions_enable_all();
}

safe_early void smp_cpu_cfg(size_t context_id)
{
    (void)context_id;

    // the entry happens with the mmu disabled
    mmu_core_handle* ch = as_kpa(mm_mmu_core_handler_get_self());

    bool res = mmu_core_handle_new(
        ch,
        as_kpa(MM_MMU_IDENTITY_LO_MAPPING),
        as_kpa(MM_MMU_KERNEL_MAPPING),
        true,
        true,
        true,
        true,
        false);
    ASSERT(res);

    mmu_core_activate(ch);

    extern uint64_t _el1_vector_table[];
    sysreg_write(VBAR_EL1, _el1_vector_table);

    mm_reloc(as_kva((void*)smp_cfg_end));
}

extern void            kernel_entry();
safe_early static void smp_cfg_end()
{
    mmu_core_set_mapping(mm_mmu_core_handler_get_self(), MM_MMU_UNMAPPED_LO);
    mmu_core_set_mapping(mm_mmu_core_handler_get_self(), MM_MMU_KERNEL_MAPPING);

    kernel_cpu_local_init();

    init_devices();

    ksleep(1e6);

    _reset_stack_and_branch(aloc_stack(MEM_MiB(4)), kernel_entry);
}
