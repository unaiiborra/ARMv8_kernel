// The purpose of this file is to manually set the initialization stages of the
// drivers, as a driver can need to initialize before and after irqs are enabled

#include <arm/cpu.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/tmu/tmu.h>
#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/init.h>
#include <lib/stdmacros.h>

#include "drivers/arm_generic_timer/arm_generic_timer.h"

// TODO: full remake of this file


static void uart_driver_init()
{
    uart_init(&UART2_DRIVER);
}



static void uart_register_irq()
{
    GICV3_init_irq(
        &GIC_DRIVER,
        irq_id_new(IMX8MP_IRQ_UART2),
        0x80,
        GICV3_LEVEL_SENSITIVE,
        arm_get_cpu_affinity());
}

KERNEL_INITCALL(uart_driver_init);
KERNEL_INITCALL(uart_register_irq);



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
    GICV3_init_irq(
        &GIC_DRIVER,
        irq_id_new(IMX8MP_IRQ_ANAMIX_TEMP),
        0x0,
        GICV3_LEVEL_SENSITIVE,
        arm_get_cpu_affinity());
}

KERNEL_INITCALL(tmu_driver_init);
KERNEL_INITCALL(tmu_irq_register);


static void agt_driver_init()
{
    AGT_init(&AGT0_DRIVER);
}

static void agt_irq_register()
{
    GICV3_enable_ppi(&GIC_DRIVER, irq_id_new(27), arm_get_cpu_affinity());
}

KERNEL_INITCALL(agt_driver_init);
KERNEL_INITCALL(agt_irq_register);
