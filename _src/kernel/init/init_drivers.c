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


static void uart_stage0_()
{
    uart_init_stage0(&UART2_DRIVER);
}

static void uart_stage1_()
{
    uart_init_stage1(&UART2_DRIVER);
}

static void uart_stage2_()
{
    GICV3_init_irq(
        &GIC_DRIVER,
        irq_id_new(IMX8MP_IRQ_UART2),
        0x80,
        GICV3_LEVEL_SENSITIVE,
        arm_get_cpu_affinity());
}

KERNEL_INITCALL(uart_stage0_, KERNEL_INITCALL_STAGE0);
KERNEL_INITCALL(uart_stage1_, KERNEL_INITCALL_STAGE1);
KERNEL_INITCALL(uart_stage2_, KERNEL_INITCALL_STAGE2);

static void tmu_stage0_()
{
    TMU_init_stage0(
        &TMU_DRIVER,
        (tmu_cfg) {
            .warn_max     = 40,
            .critical_max = 85,
        });
}

static void tmu_stage1_()
{
    TMU_init_stage1(&TMU_DRIVER);
}

static void tmu_stage2_()
{
    GICV3_init_irq(
        &GIC_DRIVER,
        irq_id_new(IMX8MP_IRQ_ANAMIX_TEMP),
        0x0,
        GICV3_LEVEL_SENSITIVE,
        arm_get_cpu_affinity());
}

KERNEL_INITCALL(tmu_stage0_, KERNEL_INITCALL_STAGE0);
KERNEL_INITCALL(tmu_stage1_, KERNEL_INITCALL_STAGE1);
KERNEL_INITCALL(tmu_stage2_, KERNEL_INITCALL_STAGE2);

static void agt_stage0_()
{
    AGT_init_stage0(&AGT0_DRIVER);
}
static void agt_stage2_()
{
    GICV3_enable_ppi(&GIC_DRIVER, irq_id_new(27), arm_get_cpu_affinity());
}
KERNEL_INITCALL(agt_stage0_, KERNEL_INITCALL_STAGE0);
KERNEL_INITCALL(agt_stage2_, KERNEL_INITCALL_STAGE2);
