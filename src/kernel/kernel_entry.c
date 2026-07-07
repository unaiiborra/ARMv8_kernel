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
    }
    else {
        printf("Hello from core %d\n\r", get_cpuid());
        scheduler_loop_cpu_enter();
        printf("\n\rExited %d!\n\r", get_cpuid());

        loop asm volatile("wfi");
    }

    task_t* proc_a = task_new("Process A");
    task_t* proc_b = task_new("Process B");

    uintptr_t entry_a, entry_b;

    elf_load(
        proc_a,
        EMBEDDED_BINARY(pa_elf),
        EMBEDDED_BINARY_SIZE(pa_elf),
        &entry_a);

    elf_load(
        proc_b,
        EMBEDDED_BINARY(pb_elf),
        EMBEDDED_BINARY_SIZE(pb_elf),
        &entry_b);

    schedule_ready_thread(proc_a, entry_a);
    schedule_ready_thread(proc_b, entry_b);

    smp_init();

    scheduler_loop_cpu_enter();

    printf("\n\rExited %d!\n\r", get_cpuid());

    loop asm volatile("wfi");
}
