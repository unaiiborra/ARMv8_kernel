#pragma once

#include <kernel/devices/device.h>
#include <lib/lock/spinlock.h>
#include <lib/stdbitfield.h>

// Thermal monitoring unit

extern const int8_t TMU_MAX_SENSOR_TEMP_C;
extern const int8_t TMU_MIN_SENSOR_TEMP_C;

typedef struct {
    // TODO: when mmu is enabled this alignas are not neccesary (it sends an
    // alignment exception)
    _Alignas(16) int8_t warn_max;
    _Alignas(16) int8_t critical_max;
} tmu_cfg;

typedef struct {
    _Alignas(16) spinlock_t state_lock;
    _Alignas(16) tmu_cfg cfg;
    _Alignas(16) uint8_t init_stage;
    _Alignas(16) bool warn_pending; // used for the kernel to ask if a temp
                                    // warning arrived
    _Alignas(16) bitfield8 irq_status;
} tmu_state;

void TMU_init_stage0(const driver_handle* h, tmu_cfg cfg);
void TMU_init_stage1(const driver_handle* h);

int8_t TMU_get_temp(const driver_handle* h);

void TMU_set_warn_temp(const driver_handle* h, int8_t temp_c);
bool TMU_get_warnings_enabled(const driver_handle* h);
void TMU_enable_warnings(const driver_handle* h);
void TMU_disable_warnings(const driver_handle* h);

// Tells if the warning temperature threashold was reached, calling this
// function disables the pending state. The warnings must be enabled again, as
// when a warning is received, warnings are disabled (TMU_warn_pending will
// still tell if a warning arrived, but no more warnings will be received)
bool TMU_warn_pending(const driver_handle* h);

// Critical irq allways active, TF-A will shut down the cpu automatically
void TMU_set_critical_temp(const driver_handle* h, int8_t temp_c);

void TMU_handle_irq(const driver_handle* h);
