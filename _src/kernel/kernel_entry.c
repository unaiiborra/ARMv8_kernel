#include <arm/exceptions/exceptions.h>
#include <arm/sysregs/sysregs.h>
#include <arm/tfa/smccc.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/tmu/tmu.h>
#include <kernel/init.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/userspace_embedded_examples.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdnoreturn.h>

#include "arm/cpu.h"
#include "arm/mmu.h"
#include "kernel/io/stdio.h"
#include "kernel/mm/elf.h"
#include "kernel/scheduler.h"
#include "lib/unit/mem.h"
#include "mm/mm_info.h"

// Main function of the kernel, called by the bootloader (/boot/boot.S)
noreturn void kernel_entry()
{
    size_t coreid = ARM_get_cpu_affinity().aff0;

    if (coreid == 0) {
        if (!mm_kernel_is_relocated())
            kernel_early_init();
        else
            kernel_init();
    }

    __attribute((unused)) mm_ksections y = MM_KSECTIONS;


    kprint("\n\rSTART\n\r");


    utask* ut = utask_new("testing", 4 * MEM_KiB);


    elf_load_result elf_res =
        elf_load(ut, (void*)HELLO_WORLD_ELF, HELLO_WORLD_ELF_SIZE);
    ASSERT(elf_res == ELF_LOAD_OK);


    schedule_thread(ut, ut->entry);

    scheduler_loop_cpu_enter();


    loop asm volatile("wfi");
}
