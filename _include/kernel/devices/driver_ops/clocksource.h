#pragma once

#include <stdint.h>

#include "../device.h"


// DEVICE_CLASS_CLOCKSOURCE

typedef struct {
    int32_t (*init)(driver_handle_t handle);
    int32_t (*exit)(driver_handle_t handle); // probably NULL
    uint64_t (*get_ticks)(driver_handle_t handle);
    uint64_t (*get_freq_hz)(driver_handle_t handle);
    void (*irq_handle)(driver_handle_t handle); // probably NULL
} clocksource_ops_t;



#define get_clocksource_ops(dev) \
    _Generic(                    \
        (dev),                   \
        const device_t*: (const clocksource_ops_t*)((dev)->driver_ops))
        