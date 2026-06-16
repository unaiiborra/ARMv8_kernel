#pragma once

#include <stdint.h>

#include "../device.h"


// DEVICE_CLASS_THERMAL_SENSOR

typedef struct {
    int32_t (*init)(driver_handle_t handle);
    int32_t (*exit)(driver_handle_t handle);

    float precission;
    float min_celsius;
    float max_celsius;

    float (*read_current_celsius)(driver_handle_t handle);

    int32_t (*irq_enable)(driver_handle_t handle);
    int32_t (*irq_disable)(driver_handle_t handle);

    int32_t (*irq_notify_warn_over)(
        driver_handle_t handle,
        float           threshold,
        void (*handler)(void* ctx),
        void* ctx);

    int32_t (*irq_notify_warn_under)(
        driver_handle_t handle,
        float           threshold,
        void (*handler)(void* ctx),
        void* ctx);

    int32_t (*irq_notify_critical_over)(
        driver_handle_t handle,
        float           threshold,
        void (*handler)(void* ctx),
        void* ctx);

    int32_t (*irq_notify_critical_under)(
        driver_handle_t handle,
        float           threshold,
        void (*handler)(void* ctx),
        void* ctx);

    void (*irq_handle)(driver_handle_t handle);
} thermal_sensor_ops_t;


#define get_thermal_sensor_ops(dev) \
    _Generic(                       \
        (dev),                      \
        const device_t*: (const thermal_sensor_ops_t*)((dev)->driver_ops))
