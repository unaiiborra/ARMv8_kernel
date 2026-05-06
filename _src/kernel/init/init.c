#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <drivers/gicv3.h>
#include <kernel/devices/device.h>
#include <kernel/devices/drivers.h>
#include <kernel/exception/irq.h>
#include <kernel/init.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <kernel/time.h>
#include <stddef.h>
#include <target/imx8mp.h>



extern kernel_initcall_t __kernel_initcalls_start[];
extern kernel_initcall_t __kernel_initcalls_end[];


extern void rust_kernel_initcalls(void);


static bool kernel_initialized = false;

void kernel_init(void)
{
    // TODO: reorder initialization stages, improve the irq initialization stage
    // and document it

    mm_init(); // init kmalloc, cache malloc, etc.
    scheduler_init();
    device_ctrl_init();
    irq_ctrl_init();

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


    const device_t* irq_ctrl = device_get_primary(DEVICE_CLASS_IRQ_CTRL);
    ASSERT(irq_ctrl);
    const irq_ctrl_ops_t* irq_ops = (irq_ctrl_ops_t*)irq_ctrl->driver_ops;

    driver_handle_t handle = device_get_driver_handle(irq_ctrl);

    irq_ops->init(handle);
    irq_ops->init_cpu(handle, arm_get_cpu_affinity().aff0);

    io_init(); // init kprint, kprintf...

#ifdef DEBUG_DUMP
    term_prints("Identity mapping mmu: \n\r");
    mm_dbg_print_mmu();
#endif

    time_ctrl_init(); // clock and timer ctrl init

    kernel_initialized = true;
}


bool kernel_is_initialized()
{
    return kernel_initialized;
}