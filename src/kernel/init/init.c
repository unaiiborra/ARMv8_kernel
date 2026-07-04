#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <drivers/gicv3.h>
#include <kernel/devices/device.h>
#include <kernel/exception/irq.h>
#include <kernel/init.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <kernel/time.h>
#include <lib/performance_monitor.h>
#include <stddef.h>
#include <target/imx8mp.h>



extern kernel_initcall_t __kernel_initcalls_start[];
extern kernel_initcall_t __kernel_initcalls_end[];
extern kernel_initcall_t __kernel_cpu_initcalls_start[];
extern kernel_initcall_t __kernel_cpu_initcalls_end[];

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
    performance_monitor_init();

    const device_t* irq_ctrl = device_get_primary(DEVICE_CLASS_IRQ_CTRL);
    ASSERT(irq_ctrl);
    const irq_ctrl_ops_t* irq_ops = (irq_ctrl_ops_t*)irq_ctrl->driver_ops;
    driver_handle_t       handle  = device_get_driver_handle(irq_ctrl);

    irq_ops->init(handle);

    for (kernel_initcall_t* fn = __kernel_initcalls_start;
         fn < __kernel_initcalls_end;
         fn++)
        (*fn)();

    io_init(); // init print, printf...

    kernel_cpu_local_init();

#ifdef DEBUG_DUMP
    term_prints("Identity mapping mmu: \n\r");
    mm_dbg_print_mmu();
#endif

    kernel_initialized = true;

    arm_exceptions_enable_all();
}


bool kernel_is_initialized()
{
    return kernel_initialized;
}


void kernel_cpu_local_init()
{
    for (kernel_initcall_t* fn = __kernel_cpu_initcalls_start;
         fn < __kernel_cpu_initcalls_end;
         fn++)
        (*fn)();

    const device_t* irq_ctrl = device_get_primary(DEVICE_CLASS_IRQ_CTRL);
    ASSERT(irq_ctrl);
    const irq_ctrl_ops_t* irq_ops = (irq_ctrl_ops_t*)irq_ctrl->driver_ops;
    driver_handle_t       handle  = device_get_driver_handle(irq_ctrl);

    irq_ops->init_cpu(handle, arm_get_cpu_affinity().aff0);

    time_ctrl_init_cpu(); // clock and timer ctrl init
}
