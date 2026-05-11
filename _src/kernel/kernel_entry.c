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
    uintptr_t       newstl_entry;


    task_t* newstl = task_new("newstl", 4 * MEM_KiB);


    elf_res = elf_load(
        newstl,
        EMBEDDED_BINARY(newstl_elf),
        EMBEDDED_BINARY_SIZE(newstl_elf),
        &newstl_entry);
    ASSERT(elf_res == ELF_LOAD_OK);


    schedule_ready_thread(newstl, newstl_entry);
    scheduler_loop_cpu_enter();


    loop asm volatile("wfi");
}
