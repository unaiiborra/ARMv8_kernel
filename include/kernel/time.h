#pragma once


#include <kernel/devices/device.h>
#include <stdint.h>

typedef int64_t timepoint_t;
typedef int64_t duration_ns_t;


/* Clock (includes a clocksource and supports a related timer) */

typedef struct clock clock_t;


extern clock_t* const HRTIMER;


void time_ctrl_init();


/*
   clock is an abstraction that allows both setting up timers and getting
   timepoints. It requires a clocksource and a timer driver that are related in
   frequency and ticks to work (hardware related). If a NULL timer driver is
   provided, it will only allow getting timepoints. The clocksource cannot be
   NULL. Using the driver ops directly when they are registered to a clock can
   cause ub as the clock might have set timer irq events, so it should be
   handled directly from the clock. It allows for virtual timer events (multiple
   events creation even if only one event is allowed from hardware side)
*/
clock_t* clock_new(
    const device_t* clocksource,
    const device_t* timer,
    timepoint_t     current_time,
    bool mutable);


clock_t* clock_new_offset(
    const device_t* clocksource,
    const device_t* timer,
    duration_ns_t   offset /*
                            * offset from the clocksource to the current time,
                            * zero to mantain exactly the clocksource time
                            */
    ,
    bool mutable);

void clock_delete(clock_t* clock);

/// sets the current time, it will panic if the clock is immutable
void        clock_set_time(clock_t* clock, timepoint_t current_time);
timepoint_t clock_now(clock_t* clock);

duration_ns_t clock_time_until(clock_t* clock, timepoint_t tp);




/* Core local timer */

typedef void (*timer_callback_t)(void* ctx);

typedef struct {
    uint64_t    event_id;
    timepoint_t expiration_point;
    clock_t*    clock;
} timer_event_t;

timer_event_t timer_create_event(
    clock_t*         clock,
    timer_callback_t cb,
    void*            ctx,
    timepoint_t      t);

timer_event_t timer_create_event_delta(
    clock_t*         clock,
    timer_callback_t cb,
    void*            ctx,
    duration_ns_t    delta_ns);

bool timer_cancel_event(timer_event_t event);
