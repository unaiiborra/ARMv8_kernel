#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <arm/smccc/smc.h>
#include <arm/sysregs/sysregs.h>
#include <drivers/gicv3.h>
#include <kernel/embedded_binary.h>
#include <kernel/init.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/smp.h>
#include <lib/data_structures/kvec.h>
#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "kernel/mm/elf.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"

noreturn void kernel_entry()
{
    if (get_cpuid() == 0) {
        if (!mm_kernel_is_relocated())
            kernel_early_init();
        else
            kernel_init();

        smp_init();

        dbg_printf(DEBUG_LOG, "Core %d initialized\n\r", get_cpuid());
    }
    else {
        dbg_printf(DEBUG_LOG, "Core %d initialized\n\r", get_cpuid());
        scheduler_loop_cpu_enter();
        dbg_printf(DEBUG_LOG, "Runqueue %d exited succesfully\n\r", get_cpuid());

        loop asm volatile("wfi");
    }

    task_t* proc_a = task_new("Process A");
    task_t* proc_b = task_new("Process B");

    uintptr_t entry_a, entry_b;

    elf_load(
        proc_a,
        EMBEDDED_BINARY(demo_a_elf),
        EMBEDDED_BINARY_SIZE(demo_a_elf),
        &entry_a);

    elf_load(
        proc_b,
        EMBEDDED_BINARY(demo_b_elf),
        EMBEDDED_BINARY_SIZE(demo_b_elf),
        &entry_b);

    schedule_ready_thread(proc_a, entry_a);
    schedule_ready_thread(proc_b, entry_b);

    scheduler_loop_cpu_enter();

    dbg_printf(DEBUG_LOG, "Runqueue %d exited succesfully\n\r", get_cpuid());

    loop asm volatile("wfi");
}
