#pragma once
// This header defines an abstraction interface for drivers, where the kernel
// does not have to know the internals of each driver

#include <kernel/devices/drivers.h>
#include <lib/mem.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/mm.h"


typedef struct {
    vuintptr_t   base;
    void** const state;
} driver_handle_t;


typedef enum {
    DEVICE_CLASS_GENERIC = 0,
    DEVICE_CLASS_IRQ_CTRL,
    DEVICE_CLASS_SERIAL,
    DEVICE_CLASS_TIMER,
    DEVICE_CLASS_CLOCKSOURCE,

    DEVICE_CLASS_COUNT,
} device_class_t;



typedef struct {
    uint64_t       uid;
    const char*    name;
    uint8_t        rank;
    device_class_t class_id;
    puintptr_t     base_pa;
    void*          driver_state;
    const void*    driver_ops;
} device_t;


void device_ctrl_init();

void device_register(
    const char*    name,
    device_class_t class_id,
    uint8_t        rank,
    puintptr_t     base,
    const void*    driver_ops);
const device_t* device_get_by_name(device_class_t class_id, const char* name);
const device_t* device_get_primary(device_class_t class_id);


[[gnu::always_inline]] static inline driver_handle_t device_get_driver_handle(
    const device_t* device)
{
    return (driver_handle_t) {
        .base  = mm_kernel_is_relocated() ? kpa_to_kva(device->base_pa)
                                          : device->base_pa,
        .state = (void**)&device->driver_state,
    };
}