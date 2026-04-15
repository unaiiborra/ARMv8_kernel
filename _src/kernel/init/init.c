#include <arm/exceptions/exceptions.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <kernel/devices/drivers.h>
#include <kernel/init.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/scheduler.h"
extern kernel_initcall_t __kernel_init_stage0_start[];
extern kernel_initcall_t __kernel_init_stage0_end[];

extern kernel_initcall_t __kernel_init_stage1_start[];
extern kernel_initcall_t __kernel_init_stage1_end[];

extern kernel_initcall_t __kernel_init_stage2_start[];
extern kernel_initcall_t __kernel_init_stage2_end[];

extern void rust_kernel_initcalls_stage0(void);
extern void rust_kernel_initcalls_stage1(void);
extern void rust_kernel_initcalls_stage2(void);

KERNEL_INITCALL(rust_kernel_initcalls_stage0, KERNEL_INITCALL_STAGE0);
KERNEL_INITCALL(rust_kernel_initcalls_stage1, KERNEL_INITCALL_STAGE1);
KERNEL_INITCALL(rust_kernel_initcalls_stage2, KERNEL_INITCALL_STAGE2);


static bool kernel_initialized = false;

void kernel_init(void)
{
    io_init(); // init kprint, kprintf...
    mm_init(); // init kmalloc, cache malloc, etc.
    scheduler_init();

    // Stage 0 (pre irq initialization)
    for (kernel_initcall_t* fn = __kernel_init_stage0_start;
         fn < __kernel_init_stage0_end;
         fn++)
        (*fn)();

    arm_exceptions_set_status((arm_exception_status) {
        .fiq    = true,
        .irq    = true,
        .serror = true,
        .debug  = true,
    });


    GICV3_init_distributor(&GIC_DRIVER);
    GICV3_init_cpu(&GIC_DRIVER, arm_get_cpu_affinity().aff0);

    for (kernel_initcall_t* fn = __kernel_init_stage1_start;
         fn < __kernel_init_stage1_end;
         fn++)
        (*fn)();

    for (kernel_initcall_t* fn = __kernel_init_stage2_start;
         fn < __kernel_init_stage2_end;
         fn++)
        (*fn)();


#ifdef DEBUG_DUMP
    term_prints("Identity mapping mmu: \n\r");
    mm_dbg_print_mmu();
#endif

    kernel_initialized = true;
}


bool kernel_is_initialized()
{
    return kernel_initialized;
}