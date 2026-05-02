// The purpose of this file is to manually set the initialization stages of the
// drivers, as a driver can need to initialize before and after irqs are enabled
// NOTE: This should be actually managed by with a dtb file or similar, not
// semihardcoded like it is, but because writing a module for dtb interpretation
// escapes the current scope of the project, I just define it directly

#include <arm/cpu.h>
#include <drivers/arm_generic_timer.h>
#include <drivers/imx8mp_tmu.h>
#include <drivers/imx8mp_uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/init.h>
#include <lib/stdmacros.h>
#include <target/device_map.h>

#include "kernel/devices/device.h"
#include "kernel/devices/driver_ops/irq_ctrl.h"
#include "kernel/exception/irq.h"
#include "target/imx8mp.h"


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
        IMX8MP_UART_OPS,
        TRIGGER_LEVEL_SENSITIVE,
        arm_get_cpu_affinity_as_u32(),
        150);
}

KERNEL_INITCALL(uart_driver_register);

// ---------------------------------------------------------------------------
// TMU
// ---------------------------------------------------------------------------

static void tmu_driver_register()
{
    device_register(
        "imx8mp/thermal-monitoring-unit",
        DEVICE_CLASS_THERMAL_SENSOR,
        255,
        UART2_BASE,
        IMX8MP_THERMAL_MONITORING_UNIT_OPS);

    irq_register_driver(
        IMX8MP_IRQ_ANAMIX_TEMP,
        "imx8mp/thermal-monitoring-unit",
        DEVICE_CLASS_THERMAL_SENSOR,
        IMX8MP_THERMAL_MONITORING_UNIT_OPS,
        TRIGGER_LEVEL_SENSITIVE,
        arm_get_cpu_affinity_as_u32(),
        0); // Max priority
}

KERNEL_INITCALL(tmu_driver_register);


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
        ARM_GENERIC_TIMER_OPS,
        TRIGGER_LEVEL_SENSITIVE,
        arm_get_cpu_affinity_as_u32(),
        90);
}

KERNEL_INITCALL(agt_register);