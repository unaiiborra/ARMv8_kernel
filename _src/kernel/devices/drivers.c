#include <drivers/tmu/tmu.h>
#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <lib/stdmacros.h>
#include <target/device_map.h>

#include "drivers/arm_generic_timer/arm_generic_timer.h"
#include "kernel/mm.h"




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
