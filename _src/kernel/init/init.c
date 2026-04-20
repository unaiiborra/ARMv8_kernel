#include <arm/exceptions/exceptions.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <kernel/devices/drivers.h>
#include <kernel/init.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <stddef.h>



extern kernel_initcall_t __kernel_initcalls_start[];
extern kernel_initcall_t __kernel_initcalls_end[];


extern void rust_kernel_initcalls(void);


static bool kernel_initialized = false;

void kernel_init(void)
{
    io_init(); // init kprint, kprintf...
    mm_init(); // init kmalloc, cache malloc, etc.
    scheduler_init();


    for (kernel_initcall_t* fn = __kernel_initcalls_start;
         fn < __kernel_initcalls_end;
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