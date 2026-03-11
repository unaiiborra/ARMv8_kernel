#pragma once
// This header defines an abstraction interface for drivers, where the kernel
// does not have to know the internals of each driver

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uintptr_t base;
    void* state;
} driver_handle;

typedef struct {
    uint64_t id;
    const char* name;
    uint64_t irqid;
    const driver_handle* drv;
} kernel_device;
