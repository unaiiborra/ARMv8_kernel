#include <arm/exceptions/exceptions.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <drivers/interrupts/gicv3/raw/gicv3_raw.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>

#include "drivers/interrupts/gicv3/raw/gicd_typer.h"
#include "drivers/interrupts/gicv3/raw/gicr_waker.h"

// TODO: Remake everything to make it decent

// Declared at gicv3_arm_interface.S
extern void     _GICV3_ARM_ICC_SRE_EL1_write(uint64_t v);
extern void     _GICV3_ARM_ICC_PMR_EL1_write(uint64_t v);
extern void     _GICV3_ARM_ICC_IGRPEN1_EL1_write(uint64_t v);
extern void     _GICV3_ARM_ICC_EOIR1_EL1_write(uint64_t v);
extern uint64_t _GICV3_ARM_ICC_IAR1_EL1_read(void);

void GICV3_set_cpu_priority_threshold(uint8_t threshold)
{
    _GICV3_ARM_ICC_PMR_EL1_write(threshold);
}

static inline void GICV3_arm_interface_enable_(void)
{
    // Enable system register interface
    _GICV3_ARM_ICC_SRE_EL1_write(1);
    _GICV3_ARM_ICC_PMR_EL1_write(0xFF);
    _GICV3_ARM_ICC_IGRPEN1_EL1_write(1);
}

/// If irq number is not valid panics
static void GICV3_validate_spi_id_(const driver_handle* h, irq_id id)
{
    GicdTyper typer   = GICV3_GICD_TYPER_read(h->base);
    uint32_t  itlines = (uint32_t)GICV3_GICD_TYPER_ITLinesNumber_get(typer);

    uint32_t max_spi = (32 * (itlines + 1)) - 1;

    if (id.n < 32 || id.n > max_spi)
        PANIC("Invalid SPI INTID");
}

void GICV3_set_spi_group1ns(const driver_handle* h, irq_id id, bool v)
{
    // imx8mp irqs start from 32 as 0..31 in the gic are reserved
    GICV3_validate_spi_id_(h, id);

    uint32_t n   = ((uint32_t)id.n) / 32;
    uint32_t bit = ((uint32_t)id.n) % 32;

    GicdIgroupr igroupr = GICV3_GICD_IGROUPR_read(h->base, n);
    GICV3_GICD_IGROUPR_BF_set(&igroupr, bit, v);
    GICV3_GICD_IGROUPR_write(h->base, n, igroupr);
}

void GICV3_route_spi_to_cpu(
    const driver_handle* h,
    irq_id               id,
    arm_cpu_affinity     affinity)
{
    GICV3_validate_spi_id_(h, id);

    GicdIrouter r = {0};

    GICV3_GICD_IROUTER_Interrupt_Routing_Mode_set(&r, false);

    GICV3_GICD_IROUTER_Aff3_set(&r, affinity.aff3);
    GICV3_GICD_IROUTER_Aff2_set(&r, affinity.aff2);
    GICV3_GICD_IROUTER_Aff1_set(&r, affinity.aff1);
    GICV3_GICD_IROUTER_Aff0_set(&r, affinity.aff0);

    GICV3_GICD_IROUTER_write(h->base, id.n, r);
}

void GICV3_route_spi_to_self(const driver_handle* h, irq_id id)
{
    GICV3_route_spi_to_cpu(h, id, arm_get_cpu_affinity());
}

void GICV3_enable_spi(const driver_handle* h, irq_id id)
{
    GICV3_validate_spi_id_(h, id);

    uint32_t n   = ((uint32_t)id.n) / 32;
    uint32_t bit = ((uint32_t)id.n) % 32;

    GICV3_GICD_ISENABLER_set_bit(h->base, n, bit);
}

void GICV3_set_priority(const driver_handle* h, irq_id id, uint8_t priority)
{
    GICV3_validate_spi_id_(h, id);

    uint32_t n        = ((uint32_t)id.n) / 4;
    uint32_t byte_idx = ((uint32_t)id.n) % 4;

    GicdIpriority r = GICV3_GICD_IPRIORITYR_read(h->base, n);
    GICV3_GICD_IPRIORITYR_BF_set(&r, byte_idx, priority);
    GICV3_GICD_IPRIORITYR_write(h->base, n, r);
}

void GICV3_set_edge_triggered(const driver_handle* h, irq_id id)
{
    GICV3_validate_spi_id_(h, id);

    uint32_t n    = ((uint32_t)id.n) / 16;
    uint32_t slot = ((uint32_t)id.n) % 16;

    GicdIcfgr r = GICV3_GICD_ICFGR_read(h->base, n);
    GICV3_GICD_ICFGR_set(&r, slot, 0b10);
    GICV3_GICD_ICFGR_write(h->base, n, r);
}

void GICV3_set_level_sensitive(const driver_handle* h, irq_id id)
{
    GICV3_validate_spi_id_(h, id);

    uint32_t n    = ((uint32_t)id.n) / 16;
    uint32_t slot = ((uint32_t)id.n) % 16;

    GicdIcfgr r = GICV3_GICD_ICFGR_read(h->base, n);
    GICV3_GICD_ICFGR_set(&r, slot, 0b00);
    GICV3_GICD_ICFGR_write(h->base, n, r);
}

void GICV3_wake_redistributor(const driver_handle* h, size_t n)
{
    GicrWaker w = GICV3_GICR_WAKER_read(h->base, n);

    GICV3_GICR_WAKER_ProcessorSleep_set(&w, false);
    GICV3_GICR_WAKER_write(h->base, n, w);

    bool asleep = true;

    while (asleep) {
        for (size_t i = 0; i < 5000; i++)
            asm volatile("nop");

        GicrWaker r = GICV3_GICR_WAKER_read(h->base, n);
        asleep      = GICV3_GICR_WAKER_ChildrenAsleep_get(r);
    }
}

void GICV3_init_distributor(const driver_handle* h)
{
    GICV3_GICD_CTLR_write(h->base, (GicdCtlr) {.val = 0}); // disable
    asm volatile("dsb sy");

    // TODO: clean pending

    GicdCtlr ctlr = {0};
    GICV3_GICD_CTLR_EnableGrp1NS_set(&ctlr, true);
    GICV3_GICD_CTLR_write(h->base, ctlr);

    asm volatile("dsb sy");
}

void GICV3_init_cpu(const driver_handle* h, size_t cpu)
{
    GICV3_wake_redistributor(h, cpu);
    GICV3_arm_interface_enable_();
}

void GICV3_init_irq(
    const driver_handle* h,
    irq_id               id,
    uint8_t              priority,
    gicv3_irq_trigger    trigger,
    arm_cpu_affinity     cpu)
{
    GICV3_set_spi_group1ns(h, id, true);

    switch (trigger) {
        case GICV3_LEVEL_SENSITIVE:
            GICV3_set_level_sensitive(h, id);
            break;

        case GICV3_EDGE_TRIGGERED:
            GICV3_set_edge_triggered(h, id);
            break;

        default:
            PANIC("Invalid enum");
            break;
    }

    GICV3_set_priority(h, id, priority);

    GICV3_route_spi_to_cpu(h, id, cpu);

    GICV3_enable_spi(h, id);
}

irq_id GICV3_get_intid_el1()
{
    return irq_id_new(_GICV3_ARM_ICC_IAR1_EL1_read() & 0xFFFFFF);
}

void GICV3_ack_intid_el1(irq_id id)
{
    _GICV3_ARM_ICC_EOIR1_EL1_write(id.n);
}

void GICV3_enable_ppi(const driver_handle* h, irq_id id, arm_cpu_affinity cpu)
{
    size_t rd = cpu.aff0;

    uint32_t bit = id.n;

    GicrIgroupr0 ig = GICV3_GICR_IGROUPR0_read(h->base, rd);

    GICV3_GICR_IGROUPR0_set_bit(&ig, bit, true);
    GICV3_GICR_IGROUPR0_write(h->base, rd, ig);

    uint32_t n    = id.n / 4;
    uint32_t byte = id.n % 4;

    GicrIpriorityr pr = GICV3_GICR_IPRIORITYR_read(h->base, rd, n);
    GICV3_GICR_IPRIORITYR_BF_set(&pr, byte, 0x80);
    GICV3_GICR_IPRIORITYR_write(h->base, rd, n, pr);

    GICV3_GICR_ISENABLER0_set_bit(h->base, rd, bit);
}
