#pragma once

#include <stdint.h>

#include "kernel/devices/device.h"
#include "kernel/devices/drivers.h"
#include "kernel/smp.h"

typedef void (*irq_std_handler_t)(void* ctx);
typedef void (*irq_driver_handler_t)(driver_handle_t handle);

typedef union {
    void*                any;
    irq_std_handler_t    std_handler;
    irq_driver_handler_t driver_handler;
} irq_handler_t;



void irq_register(
    uint32_t               irq_id,
    irq_std_handler_t      handler,
    void*                  ctx,
    irq_ctrl_ops_trigger_t trigger,
    cpuid_t                target_cpu,
    uint8_t                priority);

void irq_register_driver(
    uint32_t       irq_id,
    const char*    driver_name,
    device_class_t driver_class,
    void (*driver_irq_handler)(driver_handle_t handle),
    irq_ctrl_ops_trigger_t trigger,
    cpuid_t                target_cpu,
    uint8_t                priority);

void irq_unregister(uint32_t irq_id);

// called from the exception vector
void irq_dispatch();