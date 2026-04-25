#pragma once


#include <kernel/devices/device.h>
#include <kernel/devices/driver_ops/clocksource.h>
#include <kernel/devices/driver_ops/timer.h>
#include <lib/lock.h>
#include <stddef.h>
#include <stdint.h>


void AGT_init(const driver_handle* h);
void AGT_handle_irq(const driver_handle* h);

/*
 *      Counter
 */

uint64_t AGT_ns_to_cycles(uint64_t ns);
uint64_t AGT_us_to_cycles(uint64_t us);

// Raw counter (ticks)
uint64_t AGT_cnt_cycles(void);

// Counter frequency (Hz)
uint64_t AGT_cnt_freq(void);

uint64_t AGT_cnt_time_ns(void);

uint64_t AGT_cnt_time_us(void);

/*
 *      Timer
 */

/// Callbacks must be irq safe and non blocking. Calling the driver functions
/// from the callback is allowed, as the driver will defer the scheduling of the
/// new timer. The callback will be called inside an irq, so it should be short.
typedef void* timer_arg;
typedef void (*timer_cb_t)(timer_arg);

typedef struct {
    spinlock_t lock;
    timer_cb_t timer_cb;
    timer_arg  arg;
    bool       timer_fired;
    // As calling the functions that require the state of the driver locks the
    // state, calling from the callback those functions again will result in an
    // infinite lock. Because of that, the driver checks and "emulates" the
    // expected behaviour of what should happen with calls outside the callback
    // by defering the actions. Thanks to this, calling the driver functions
    // from the callback is safe
    struct {
        spinlock_t under_callback_gate; // Locked if not under callback
        bool       under_cb_scheduled;
        timer_cb_t under_cb_timer_cb;
        timer_arg  under_cb_arg;
        uint64_t   cycles_v;
    } defer_cb;
} agt_state;

/// Schedules the timer, returns if the timer was already set and it was
/// overrided.
bool AGT_timer_schedule_delta(
    const driver_handle* h,
    uint64_t             delta_ns,
    timer_cb_t           cb,
    timer_arg            arg);

/// Schedules the timer, returns if the timer was already set and it was
/// overrided.
bool AGT_timer_schedule_cycles(
    const driver_handle* h,
    uint64_t             cycles,
    timer_cb_t           cb,
    timer_arg            arg);

/// Returns is a timer has been scheduled, it does not represent the state of
/// the hardware 1:1
bool AGT_timer_is_armed(const driver_handle* h);

/// checking this will automatically ack the flag. Scheduling a new timer will
/// also reset the firing flag
bool AGT_timer_has_fired(const driver_handle* h);

/// cancels the scheduled timer
void AGT_timer_cancel(const driver_handle* h);


extern const timer_ops_t* const       ARM_GENERIC_TIMER_OPS;
extern const clocksource_ops_t* const ARM_GENERIC_TIMER_CLOCKSOURCE_OPS;
