#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <arm/smccc/smc.h>
#include <arm/sysregs/sysregs.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/tmu/tmu.h>
#include <kernel/init.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/userspace_embedded_examples.h>
#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "kernel/io/stdio.h"
#include "kernel/mm/elf.h"
#include "kernel/scheduler.h"
#include "kernel/smp.h"
#include "lib/unit/mem.h"


// Main function of the kernel, called by the bootloader (/boot/boot.S)
noreturn void kernel_entry()
{
    if (get_cpuid() == 0) {
        if (!mm_kernel_is_relocated())
            kernel_early_init();
        else
            kernel_init();
    }
    else
        goto secondary_core;


    kprint("\n\rSTART\n\r");
    uint64_t spsr = sysreg_read(spsr_el1);

    kprintf("%p", spsr);


    smp_init();

secondary_core:
    for (size_t i = 0; i < 10; i++)
        kprintf("Hello from core %d\n\r", get_cpuid());

    elf_load_result              elf_res;
    attr(maybe_unused) uintptr_t hello_world_entry, print_a_entry,
        print_b_entry, multithreading_entry;

    task* hello_world    = task_new("hello_world", 4 * MEM_KiB);
    task* print_a        = task_new("print_A", 4 * MEM_KiB);
    task* print_b        = task_new("print_B", 4 * MEM_KiB);
    task* multithreading = task_new("multithreading", 4 * MEM_KiB);


    elf_res = elf_load(
        hello_world,
        (void*)HELLO_WORLD_ELF,
        HELLO_WORLD_ELF_SZ,
        &hello_world_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    elf_res =
        elf_load(print_a, (void*)PRINT_A_ELF, PRINT_A_ELF_SZ, &print_a_entry);
    ASSERT(elf_res == ELF_LOAD_OK);


    elf_res =
        elf_load(print_b, (void*)PRINT_B_ELF, PRINT_B_ELF_SZ, &print_b_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    elf_res = elf_load(
        multithreading,
        (void*)MULTITHREADING_ELF,
        MULTITHREADING_ELF_SZ,
        &multithreading_entry);
    ASSERT(elf_res == ELF_LOAD_OK);


    schedule_ready_thread(hello_world, hello_world_entry);
    schedule_ready_thread(print_a, print_a_entry);
    schedule_ready_thread(print_b, print_b_entry);

    scheduler_loop_cpu_enter();


    kprint(
        "\n\rscheduler exited, entering again for multithreading test: \n\r");

    schedule_ready_thread(multithreading, multithreading_entry);

    scheduler_loop_cpu_enter();

    kprintf("\n\rscheduler exited core %d\n\r", get_cpuid());


    loop asm volatile("wfi");
}
