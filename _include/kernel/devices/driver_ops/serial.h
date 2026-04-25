#pragma once

#include <stdint.h>

#include "../device.h"


// DEVICE_CLASS_SERIAL

typedef struct {
    int32_t (*init)(driver_handle_t handle);
    int32_t (*exit)(driver_handle_t handle);

    int32_t (
        *set_baud)(driver_handle_t handle, uint32_t baud, uint32_t clock_hz);

    int32_t (*rx_is_empty)(driver_handle_t handle);
    int32_t (*tx_is_empty)(driver_handle_t handle);

    int32_t (*rx_full)(driver_handle_t handle);
    int32_t (*tx_full)(driver_handle_t handle);

    int32_t (*getc)(driver_handle_t handle);
    int32_t (*putc)(driver_handle_t handle, char c);

    int32_t rx_sz;
    int32_t tx_sz;

    int32_t (*irq_enable)(driver_handle_t handle);
    int32_t (*irq_disable)(driver_handle_t handle);

    int32_t (*irq_notify_rx)(
        driver_handle_t handle,
        uint32_t        threshold,
        void (*handler)(void* ctx),
        void* ctx);

    int32_t (*irq_notify_tx)(
        driver_handle_t handle,
        uint32_t        threshold,
        void (*handler)(void* ctx),
        void* ctx);

    void (*irq_handle)(driver_handle_t handle);
} serial_ops_t;


#define get_serial_ops(dev) \
    _Generic((dev), const device_t*: (const serial_ops_t*)((dev)->driver_ops))
    