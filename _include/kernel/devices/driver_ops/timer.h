#pragma once

#include <stdint.h>

#include "../device.h"


// DEVICE_CLASS_TIMER

typedef struct {
    int32_t (*init)(driver_handle_t handle);
    int32_t (*exit)(driver_handle_t handle);

    int32_t (*irq_enable)(
        driver_handle_t handle); // if NULL means allways active
    int32_t (*irq_disable)(
        driver_handle_t handle); // if NULL means allways active

    int32_t (*irq_notify_tick)(
        driver_handle_t handle,
        uint64_t tick, // the clocksource device (for the frequency) of the tick
                       // must be stablshed at a higher abstraction level
        void (*handler)(void* ctx), // NULL for stop
        void* ctx                   // ignored if handler == NULL
    );

    // 0 false, 1 true, < 0: error
    int32_t (*is_running)(driver_handle_t handle);

    void (*irq_handle)(driver_handle_t handle);
} timer_ops_t;


#define get_timer_ops(dev) \
    _Generic((dev), const device_t*: (const timer_ops_t*)((dev)->driver_ops))
