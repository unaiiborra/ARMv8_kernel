#include <drivers/tmu/tmu.h>
#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <lib/stdmacros.h>

#include "device_map.h"
#include "drivers/arm_generic_timer/arm_generic_timer.h"
#include "kernel/mm.h"


// UART1
static uart_state uart1_state_;
driver_handle     UART1_DRIVER = {
    .base  = UART1_BASE + KERNEL_BASE,
    .state = &uart1_state_,
};

// UART2
static uart_state uart2_state;
driver_handle     UART2_DRIVER = {
    .base  = UART2_BASE + KERNEL_BASE,
    .state = &uart2_state,
};

// UART3
static uart_state uart3_state_;
driver_handle     UART3_DRIVER = {
    .base  = UART3_BASE + KERNEL_BASE,
    .state = &uart3_state_,
};

// UART4
static uart_state uart4_state_;
driver_handle     UART4_DRIVER = {
    .base  = UART4_BASE + KERNEL_BASE,
    .state = &uart4_state_,
};

// TMU
static tmu_state tmu_state_;
driver_handle    TMU_DRIVER = {
    .base  = TMU_BASE + KERNEL_BASE,
    .state = &tmu_state_,
};

// AGT (Arm Generic Timer)
static agt_state agt_core0_state_;
driver_handle    AGT0_DRIVER = {
    .base  = false,
    .state = &agt_core0_state_,
};
