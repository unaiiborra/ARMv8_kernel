#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <arm/smccc/smc.h>
#include <arm/sysregs/sysregs.h>
#include <drivers/gicv3.h>
#include <kernel/embedded_binary.h>
#include <kernel/init.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "kernel/io/stdio.h"
#include "kernel/mm/elf.h"
#include "kernel/smp.h"



// Main function of the kernel, called by the bootloader (/boot/boot.S)
noreturn void kernel_entry()
{
    if (get_cpuid() == 0) {
        if (!mm_kernel_is_relocated())
            kernel_early_init();
        else
            kernel_init();
    }

    kprint("Hello!\n\r");

    elf_load_result elf_res;
    uintptr_t       hello_world_entry, print_a_entry, print_b_entry,
        multithreading_entry, mmap_entry, newstl_entry, test_entry;

    task_t* hello_world    = task_new("hello_world", 4 * MEM_KiB);
    task_t* print_a        = task_new("print_A", 4 * MEM_KiB);
    task_t* print_b        = task_new("print_B", 4 * MEM_KiB);
    task_t* multithreading = task_new("multithreading", 4 * MEM_KiB);
    task_t* mmap           = task_new("mmap", 4 * MEM_KiB);
    task_t* newstl         = task_new("newstl", 4 * MEM_KiB);
    task_t* test           = task_new("test", 4 * MEM_KiB);



    elf_res = elf_load(
        hello_world,
        EMBEDDED_BINARY(hello_world_elf),
        EMBEDDED_BINARY_SIZE(hello_world_elf),
        &hello_world_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    elf_res = elf_load(
        print_a,
        EMBEDDED_BINARY(print_a_elf),
        EMBEDDED_BINARY_SIZE(print_a_elf),
        &print_a_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    elf_res = elf_load(
        print_b,
        EMBEDDED_BINARY(print_b_elf),
        EMBEDDED_BINARY_SIZE(print_b_elf),
        &print_b_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    elf_res = elf_load(
        multithreading,
        EMBEDDED_BINARY(multithreading_elf),
        EMBEDDED_BINARY_SIZE(multithreading_elf),
        &multithreading_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    elf_res = elf_load(
        mmap,
        EMBEDDED_BINARY(mmap_elf),
        EMBEDDED_BINARY_SIZE(mmap_elf),
        &mmap_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    elf_res = elf_load(
        newstl,
        EMBEDDED_BINARY(newstl_elf),
        EMBEDDED_BINARY_SIZE(newstl_elf),
        &newstl_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    elf_res = elf_load(
        test,
        EMBEDDED_BINARY(test_elf),
        EMBEDDED_BINARY_SIZE(test_elf),
        &test_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    schedule_ready_thread(hello_world, hello_world_entry);
    schedule_ready_thread(print_a, print_a_entry);
    schedule_ready_thread(print_b, print_b_entry);

    scheduler_loop_cpu_enter();


    kprint(
        "\n\rscheduler exited, entering again for multithreading test: \n\r");

    schedule_ready_thread(multithreading, multithreading_entry);
    scheduler_loop_cpu_enter();



    schedule_ready_thread(newstl, newstl_entry);
    scheduler_loop_cpu_enter();

    schedule_ready_thread(test, test_entry);
    scheduler_loop_cpu_enter();

    loop asm volatile("wfi");
}
