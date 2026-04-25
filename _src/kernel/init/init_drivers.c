// The purpose of this file is to manually set the initialization stages of the
// drivers, as a driver can need to initialize before and after irqs are enabled

#include <arm/cpu.h>
#include <drivers/tmu/tmu.h>
#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/init.h>
#include <lib/stdmacros.h>
#include <target/device_map.h>

#include "drivers/arm_generic_timer/arm_generic_timer.h"
#include "kernel/devices/device.h"
#include "kernel/devices/driver_ops/irq_ctrl.h"
#include "kernel/exception/irq.h"
#include "target/imx8mp.h"

// TODO: full remake of this file — legacy wrappers hasta migrar drivers al
// nuevo sistema

// ---------------------------------------------------------------------------
// Legacy wrappers (driver_handle* → void* ctx)
// ---------------------------------------------------------------------------

// static void uart2_irq_wrapper(void* ctx)
// {
//     uart_handle_irq(ctx);
// }
static void tmu_irq_wrapper(void* ctx)
{
    TMU_handle_irq(ctx);
}

// ---------------------------------------------------------------------------
// UART
// ---------------------------------------------------------------------------

static void uart_driver_register()
{
    device_register(
        "imx8mp/uart",
        DEVICE_CLASS_SERIAL,
        255,
        UART2_BASE,
        IMX8MP_UART_OPS);

    irq_register_driver(
        IMX8MP_IRQ_UART2,
        "imx8mp/uart",
        DEVICE_CLASS_SERIAL,
        IMX8MP_UART_OPS->irq_handle,
        TRIGGER_LEVEL_SENSITIVE,
        arm_get_cpu_affinity_as_u32(),
        0x90);
}

KERNEL_INITCALL(uart_driver_register);

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
        arm_get_cpu_affinity_as_u32(),
        0x0);
}

KERNEL_INITCALL(tmu_driver_init);
KERNEL_INITCALL(tmu_irq_register);

// ---------------------------------------------------------------------------
// AGT (PPI — irq 27)
// ---------------------------------------------------------------------------

static void agt_register()
{
    device_register(
        "arm/generic-timer/clocksource",
        DEVICE_CLASS_CLOCKSOURCE,
        255,
        0x0,
        ARM_GENERIC_TIMER_CLOCKSOURCE_OPS);

    device_register(
        "arm/generic-timer/timer",
        DEVICE_CLASS_TIMER,
        255,
        0x0,
        ARM_GENERIC_TIMER_OPS);

    irq_register_driver(
        27,
        "arm/generic-timer/timer",
        DEVICE_CLASS_TIMER,
        ARM_GENERIC_TIMER_OPS->irq_handle,
        TRIGGER_LEVEL_SENSITIVE,
        arm_get_cpu_affinity_as_u32(),
        0x90);
}

KERNEL_INITCALL(agt_register);