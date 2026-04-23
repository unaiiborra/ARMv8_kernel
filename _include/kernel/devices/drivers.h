#pragma once

#pragma message("TODO: delete this file, as it is deprecated")


#include <lib/mem.h>
#include <stdint.h>

#include "device.h"

[[deprecated("register a device_t instead")]] typedef struct {
    uintptr_t base;
    void*     state;
} driver_handle;

extern const driver_handle GIC_DRIVER;

extern driver_handle UART1_DRIVER;
extern driver_handle UART2_DRIVER;
extern driver_handle UART3_DRIVER;
extern driver_handle UART4_DRIVER;

extern driver_handle TMU_DRIVER;

extern driver_handle AGT0_DRIVER;
