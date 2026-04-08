#pragma once

#include <arm/cpu.h>
#include <kernel/exception/interrupts.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/devices/device.h"

typedef struct {
    uint64_t n;
} irq_id;

static inline irq_id irq_id_new(uint64_t id)
{
    return (irq_id) {.n = id};
}

static inline bool GICV3_irq_id_is_sgi(irq_id irq)
{
    return irq.n < 16;
}
static inline bool GICV3_irq_id_is_ppi(irq_id irq)
{
    return irq.n >= 16 && irq.n < 32;
}
static inline bool GICV3_irq_id_is_spi(irq_id irq)
{
    return irq.n <= 32;
}

void GICV3_route_spi_to_cpu(
    const driver_handle* h,
    irq_id               irq,
    ARM_cpu_affinity     affinity);

void GICV3_route_spi_to_self(const driver_handle* h, irq_id irq);

void GICV3_enable_spi(const driver_handle* h, irq_id irq);

void GICV3_set_priority(const driver_handle* h, irq_id irq, uint8_t priority);

void GICV3_set_edge_triggered(const driver_handle* h, irq_id irq);
void GICV3_set_level_sensitive(const driver_handle* h, irq_id irq);

void GICV3_wake_redistributor(const driver_handle* h, size_t n);

void GICV3_init_distributor(const driver_handle* h);

void GICV3_init_cpu(const driver_handle* h, size_t cpu);

void GICV3_set_cpu_priority_threshold(uint8_t threshold);

typedef enum {
    GICV3_LEVEL_SENSITIVE,
    GICV3_EDGE_TRIGGERED,
} gicv3_irq_trigger;

irq_id GICV3_get_intid_el1();

void GICV3_ack_intid_el1(irq_id irq_token);

void GICV3_init_irq(
    const driver_handle* h,
    irq_id               irq,
    uint8_t              priority,
    gicv3_irq_trigger    trigger,
    ARM_cpu_affinity     cpu);


void GICV3_enable_ppi(const driver_handle* h, irq_id id, ARM_cpu_affinity cpu);
