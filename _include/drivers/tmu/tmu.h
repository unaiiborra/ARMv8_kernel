#pragma once

#include <kernel/devices/device.h>
#include <kernel/devices/driver_ops/thermal_sensor.h>
#include <lib/lock.h>
#include <lib/stdbitfield.h>


extern const thermal_sensor_ops_t* const IMX8MP_THERMAL_MONITORING_UNIT_OPS;