#pragma once

#include <stdint.h>

#include "../device.h"


// DEVICE_CLASS_IRQ_CTRL

typedef enum {
    TRIGGER_LEVEL_SENSITIVE = 0,
    TRIGGER_EDGE_TRIGGERED  = 1
} irq_ctrl_ops_trigger_t;

typedef struct {
    int32_t (*init)(driver_handle_t handle);
    int32_t (*init_cpu)(driver_handle_t handle, uint32_t target_cpu);

    int32_t (*irq_enable)(driver_handle_t handle, uint32_t irq);
    int32_t (*irq_disable)(driver_handle_t handle, uint32_t irq);

    int32_t (*irq_eoi)(driver_handle_t handle, uint32_t irq);

    uint32_t (*irq_ack)(driver_handle_t handle);

    int32_t (*irq_set_priority)(
        driver_handle_t handle,
        uint32_t        irq,
        uint8_t         priority);

    int32_t (*irq_set_target)(
        driver_handle_t handle,
        uint32_t        irq,
        uint32_t        target_cpu);

    int32_t (*irq_set_trigger)(
        driver_handle_t        handle,
        uint32_t               irq,
        irq_ctrl_ops_trigger_t trigger);
} irq_ctrl_ops_t;