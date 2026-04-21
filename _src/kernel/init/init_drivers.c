// The purpose of this file is to manually set the initialization stages of the
// drivers, as a driver can need to initialize before and after irqs are enabled

#include <arm/cpu.h>
#include <drivers/tmu/tmu.h>
#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/init.h>
#include <lib/stdmacros.h>

#include "drivers/arm_generic_timer/arm_generic_timer.h"
#include "kernel/exception/irq.h"
#include "kernel/io/stdio.h"
#include "kernel/mm.h"
#include "target/imx8mp.h"

// TODO: full remake of this file — legacy wrappers hasta migrar drivers al
// nuevo sistema

// ---------------------------------------------------------------------------
// Legacy wrappers (driver_handle* → void* ctx)
// ---------------------------------------------------------------------------

static void uart2_irq_wrapper(void* ctx)
{
    driver_handle* h = as_kva(ctx);
    h->base          = as_kva(h->base);
    h->state         = as_kva(h->state);

    uart_handle_irq(h);
    io_flush(IO_STDOUT);
}
static void tmu_irq_wrapper(void* ctx)
{
    driver_handle* h = as_kva(ctx);
    h->base          = as_kva(h->base);
    h->state         = as_kva(h->state);

    TMU_handle_irq(h);
}
static void agt_irq_wrapper(void* ctx)
{
    driver_handle* h = as_kva(ctx);
    h->base          = as_kva(h->base);
    h->state         = as_kva(h->state);

    AGT_handle_irq(h);
}

// ---------------------------------------------------------------------------
// UART
// ---------------------------------------------------------------------------

static void uart_driver_init()
{
    uart_init(&UART2_DRIVER);
}

static void uart_register_irq()
{
    irq_register(
        IMX8MP_IRQ_UART2,
        uart2_irq_wrapper,
        &UART2_DRIVER,
        TRIGGER_LEVEL_SENSITIVE,
        arm_get_cpu_affinity().aff0,
        0x80);
}

KERNEL_INITCALL(uart_driver_init);
KERNEL_INITCALL(uart_register_irq);

// ---------------------------------------------------------------------------
// TMU
// ---------------------------------------------------------------------------

static void tmu_driver_init()
{
    TMU_init(
        &TMU_DRIVER,
        (tmu_cfg) {
            .warn_max     = 40,
            .critical_max = 85,
        });
}

static void tmu_irq_register()
{
    irq_register(
        IMX8MP_IRQ_ANAMIX_TEMP,
        tmu_irq_wrapper,
        &TMU_DRIVER,
        TRIGGER_LEVEL_SENSITIVE,
        arm_get_cpu_affinity().aff0,
        0x0);
}

KERNEL_INITCALL(tmu_driver_init);
KERNEL_INITCALL(tmu_irq_register);

// ---------------------------------------------------------------------------
// AGT (PPI — irq 27)
// ---------------------------------------------------------------------------

static void agt_driver_init()
{
    AGT_init(&AGT0_DRIVER);
}

static void agt_irq_register()
{
    irq_register(
        27,
        agt_irq_wrapper,
        &AGT0_DRIVER,
        TRIGGER_LEVEL_SENSITIVE,
        arm_get_cpu_affinity().aff0,
        0x80);
}

KERNEL_INITCALL(agt_driver_init);
KERNEL_INITCALL(agt_irq_register);