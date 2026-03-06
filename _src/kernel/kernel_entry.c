#include <arm/exceptions/exceptions.h>
#include <arm/tfa/smccc.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/tmu/tmu.h>
#include <kernel/init.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/string.h>

#include "arm/cpu.h"
#include "kernel/io/stdio.h"
#include "kernel/process/embedded_examples.h"
#include "kernel/process/process.h"
#include "kernel/process/thread.h"
#include "mm/mm_info.h"


// Main function of the kernel, called by the bootloader (/boot/boot.S)
_Noreturn void kernel_entry()
{
    size_t coreid = ARM_get_cpu_affinity().aff0;

    if (coreid == 0) {
        if (!mm_kernel_is_relocated())
            kernel_early_init();
        else
            kernel_init();
    }

    __attribute((unused)) mm_ksections y = MM_KSECTIONS;


    proc* p_usr;
    bool res = usr_proc_new(
        &p_usr,
        (void*)&SYSC_PRINT_ELF[0],
        SYSC_PRINT_ELF_SIZE,
        "svc elf",
        true);
    ASSERT(res);

    thread_resume();


    kprint("\n\rSTART\n\r");

    loop asm volatile("wfi");
} /* kernel_entry */
