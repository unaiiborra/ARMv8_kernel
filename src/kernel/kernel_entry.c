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

#include "arm/smccc/psci.h"
#include "kernel/io/vfs_serial.h"
#include "kernel/mm/cache_malloc.h"
#include "kernel/mm/elf.h"
#include "kernel/mm/page_malloc.h"
#include "kernel/mm/vmalloc.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "kernel/time.h"
#include "lib/lock.h"
#include "lib/mem.h"
#include "lib/performance_monitor.h"


noreturn void kernel_entry()
{
    if (get_cpuid() == 0) {
        if (!mm_kernel_is_relocated())
            kernel_early_init();
        else
            kernel_init();

        smp_init();
    }
    else {
        printf("Hello from core %d\n\r", get_cpuid());

        loop asm volatile("wfi");
    }

    task_t*   hello = task_new("hello world");
    uintptr_t entry;
    elf_load(
        hello,
        EMBEDDED_BINARY(hello_elf),
        EMBEDDED_BINARY_SIZE(hello_elf),
        &entry);

    schedule_ready_thread(hello, entry);

    scheduler_loop_cpu_enter();

    loop asm volatile("wfi");
}
