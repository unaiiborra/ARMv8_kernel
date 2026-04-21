#include <arm/exceptions/exceptions.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/interrupts/gicv3/raw/gicv3_raw.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/cpu.h"
#include "drivers/interrupts/gicv3/raw/gicd_typer.h"
#include "drivers/interrupts/gicv3/raw/gicr_waker.h"
#include "kernel/devices/drivers.h"

extern void     _GICV3_ARM_ICC_SRE_EL1_write(uint64_t v);
extern void     _GICV3_ARM_ICC_PMR_EL1_write(uint64_t v);
extern void     _GICV3_ARM_ICC_IGRPEN1_EL1_write(uint64_t v);
extern void     _GICV3_ARM_ICC_EOIR1_EL1_write(uint64_t v);
extern uint64_t _GICV3_ARM_ICC_IAR1_EL1_read(void);

static inline void gicv3_arm_interface_enable_(void)
{
    _GICV3_ARM_ICC_SRE_EL1_write(1);
    _GICV3_ARM_ICC_PMR_EL1_write(0xFF);
    _GICV3_ARM_ICC_IGRPEN1_EL1_write(1);
}

static void gicv3_validate_spi_(vuintptr_t base, uint32_t irq)
{
    GicdTyper typer   = GICV3_GICD_TYPER_read(base);
    uint32_t  itlines = (uint32_t)GICV3_GICD_TYPER_ITLinesNumber_get(typer);
    uint32_t  max_spi = (32 * (itlines + 1)) - 1;

    if (irq < 32 || irq > max_spi)
        PANIC("Invalid SPI INTID");
}

// ---------------------------------------------------------------------------

static int32_t gicv3_init(driver_handle_t handle)
{
    GICV3_GICD_CTLR_write(handle.base, (GicdCtlr) {.val = 0});
    asm volatile("dsb sy");

    GicdCtlr ctlr = {0};
    GICV3_GICD_CTLR_EnableGrp1NS_set(&ctlr, true);
    GICV3_GICD_CTLR_write(handle.base, ctlr);

    asm volatile("dsb sy");

    return 0;
}

static int32_t gicv3_init_cpu(driver_handle_t handle, uint32_t target_cpu)
{
    GicrWaker w = GICV3_GICR_WAKER_read(handle.base, target_cpu);
    GICV3_GICR_WAKER_ProcessorSleep_set(&w, false);
    GICV3_GICR_WAKER_write(handle.base, target_cpu, w);

    bool asleep = true;
    while (asleep) {
        for (size_t i = 0; i < 5000; i++)
            asm volatile("nop");

        GicrWaker r = GICV3_GICR_WAKER_read(handle.base, target_cpu);
        asleep      = GICV3_GICR_WAKER_ChildrenAsleep_get(r);
    }

    gicv3_arm_interface_enable_();

    return 0;
}

static int32_t gicv3_irq_enable(driver_handle_t handle, uint32_t irq)
{
    if (irq < 32) {
        uint32_t rd = arm_get_cpu_affinity_as_u32();

        GicrIgroupr0 ig = GICV3_GICR_IGROUPR0_read(handle.base, rd);
        GICV3_GICR_IGROUPR0_set_bit(&ig, irq, true);
        GICV3_GICR_IGROUPR0_write(handle.base, rd, ig);

        GICV3_GICR_ISENABLER0_set_bit(handle.base, rd, irq);

        return 0;
    }

    uint32_t    n   = irq / 32;
    uint32_t    bit = irq % 32;
    GicdIgroupr ig  = GICV3_GICD_IGROUPR_read(handle.base, n);
    GICV3_GICD_IGROUPR_BF_set(&ig, bit, true);
    GICV3_GICD_IGROUPR_write(handle.base, n, ig);

    gicv3_validate_spi_(handle.base, irq);
    GICV3_GICD_ISENABLER_set_bit(handle.base, irq / 32, irq % 32);

    return 0;
}

static int32_t gicv3_irq_disable(driver_handle_t handle, uint32_t irq)
{
    if (irq < 32) {
        uint32_t rd = arm_get_cpu_affinity_as_u32();
        GICV3_GICR_ICENABLER0_set_bit(handle.base, rd, irq);
        
        return 0;
    }

    gicv3_validate_spi_(handle.base, irq);
    GICV3_GICD_ICENABLER_set_bit(handle.base, irq / 32, irq % 32);

    return 0;
}

static uint32_t gicv3_irq_ack(driver_handle_t handle)
{
    (void)handle;
    return (uint32_t)(_GICV3_ARM_ICC_IAR1_EL1_read() & 0xFFFFFF);
}

static int32_t gicv3_irq_eoi(driver_handle_t handle, uint32_t irq)
{
    (void)handle;
    _GICV3_ARM_ICC_EOIR1_EL1_write(irq);

    return 0;
}

static int32_t
gicv3_irq_set_priority(driver_handle_t handle, uint32_t irq, uint8_t priority)
{
    if (irq < 32) {
        uint32_t       rd   = arm_get_cpu_affinity_as_u32();
        uint32_t       n    = irq / 4;
        uint32_t       byte = irq % 4;
        GicrIpriorityr r    = GICV3_GICR_IPRIORITYR_read(handle.base, rd, n);
        GICV3_GICR_IPRIORITYR_BF_set(&r, byte, priority);
        GICV3_GICR_IPRIORITYR_write(handle.base, rd, n, r);
        return 0;
    }

    gicv3_validate_spi_(handle.base, irq);
    GicdIpriority r = GICV3_GICD_IPRIORITYR_read(handle.base, irq / 4);
    GICV3_GICD_IPRIORITYR_BF_set(&r, irq % 4, priority);
    GICV3_GICD_IPRIORITYR_write(handle.base, irq / 4, r);
    return 0;
}


static int32_t gicv3_irq_set_trigger(
    driver_handle_t        handle,
    uint32_t               irq,
    irq_ctrl_ops_trigger_t trigger)
{
    if (irq < 32) {
        if (irq < 16)
            return trigger == TRIGGER_EDGE_TRIGGERED ? 0 : -1;

        uint32_t  rd   = arm_get_cpu_affinity_as_u32();
        uint32_t  slot = irq - 16;
        GicrIcfgr r    = GICV3_GICR_ICFGR1_read(handle.base, rd);

        switch (trigger) {
            case TRIGGER_LEVEL_SENSITIVE:
                GICV3_GICR_ICFGR_set(&r, slot, 0b00);
                break;
            case TRIGGER_EDGE_TRIGGERED:
                GICV3_GICR_ICFGR_set(&r, slot, 0b10);
                break;
            default:
                return -1;
        }

        GICV3_GICR_ICFGR1_write(handle.base, rd, r);
        return 0;
    }

    gicv3_validate_spi_(handle.base, irq);
    uint32_t  n    = irq / 16;
    uint32_t  slot = irq % 16;
    GicdIcfgr r    = GICV3_GICD_ICFGR_read(handle.base, n);

    switch (trigger) {
        case TRIGGER_LEVEL_SENSITIVE:
            GICV3_GICD_ICFGR_set(&r, slot, 0b00);
            break;
        case TRIGGER_EDGE_TRIGGERED:
            GICV3_GICD_ICFGR_set(&r, slot, 0b10);
            break;
        default:
            return -1;
    }

    GICV3_GICD_ICFGR_write(handle.base, n, r);
    return 0;
}


static int32_t
gicv3_irq_set_target(driver_handle_t handle, uint32_t irq, uint32_t target_cpu)
{
    if (irq < 32)
        return target_cpu == arm_get_cpu_affinity_as_u32() ? 0 : -1;

    gicv3_validate_spi_(handle.base, irq);
    GicdIrouter r = {0};
    GICV3_GICD_IROUTER_Interrupt_Routing_Mode_set(&r, false);

    arm_cpu_affinity aff = arm_u32_as_cpu_affinity(target_cpu);
    GICV3_GICD_IROUTER_Aff0_set(&r, aff.aff0);
    GICV3_GICD_IROUTER_Aff1_set(&r, aff.aff1);
    GICV3_GICD_IROUTER_Aff2_set(&r, aff.aff2);
    GICV3_GICD_IROUTER_Aff3_set(&r, aff.aff3);

    GICV3_GICD_IROUTER_write(handle.base, irq, r);

    return 0;
}

// ---------------------------------------------------------------------------

static const irq_ctrl_ops_t OPS = {
    .init             = gicv3_init,
    .init_cpu         = gicv3_init_cpu,
    .irq_enable       = gicv3_irq_enable,
    .irq_disable      = gicv3_irq_disable,
    .irq_ack          = gicv3_irq_ack,
    .irq_eoi          = gicv3_irq_eoi,
    .irq_set_priority = gicv3_irq_set_priority,
    .irq_set_target   = gicv3_irq_set_target,
    .irq_set_trigger  = gicv3_irq_set_trigger,
};

const irq_ctrl_ops_t* const GICV3_OPS = &OPS;