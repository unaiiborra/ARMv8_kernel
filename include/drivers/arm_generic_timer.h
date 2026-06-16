#pragma once

#include <kernel/devices/driver_ops/clocksource.h>
#include <kernel/devices/driver_ops/timer.h>

extern const timer_ops_t* const       ARM_GENERIC_TIMER_OPS;
extern const clocksource_ops_t* const ARM_GENERIC_TIMER_CLOCKSOURCE_OPS;
