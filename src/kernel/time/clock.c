#include <kernel/devices/device.h>
#include <kernel/devices/driver_ops/clocksource.h>
#include <kernel/devices/driver_ops/timer.h>
#include <kernel/mm.h>
#include <kernel/time.h>
#include <lib/lock.h>
#include <stdint.h>

#include "time.h"


static inline uint64_t clocksource_now_ticks(const clock_t* clock)
{
    return get_clocksource_ops(clock->dev_clocksource)
        ->get_tick(device_get_driver_handle(clock->dev_clocksource));
}

static inline duration_ns_t clocksource_now_ns(const clock_t* clock)
{
    return tick_to_ns(clocksource_now_ticks(clock), clock->mult, clock->shift);
}


typedef struct {
    const clocksource_ops_t* ops;
    driver_handle_t          handle;
    uint32_t                 mult, shift, inv_mult, inv_shift;
} clock_init_t;

static clock_init_t init_clocksource_and_timer(
    const device_t* clocksource,
    const device_t* timer)
{
    ASSERT(clocksource != NULL, "clocksource cannot be NULL");

    const clocksource_ops_t* cs_ops    = get_clocksource_ops(clocksource);
    driver_handle_t          cs_handle = device_get_driver_handle(clocksource);

    const timer_ops_t* t_ops = get_timer_ops(timer);


    ASSERT(cs_ops->get_freq_hz != NULL && cs_ops->get_tick != NULL);

    if (cs_ops->init)
        cs_ops->init(cs_handle);

    if (cs_ops->irq_enable)
        cs_ops->irq_enable(cs_handle);

    if (t_ops) {
        driver_handle_t t_handle = device_get_driver_handle(timer);

        if (t_ops->init)
            t_ops->init(t_handle);

        if (t_ops->irq_enable)
            t_ops->irq_enable(t_handle);
    }

    uint32_t mult, shift, inv_mult, inv_shift;
    compute_mult_shift(
        cs_ops->get_freq_hz(cs_handle),
        &mult,
        &shift,
        &inv_mult,
        &inv_shift);

    return (clock_init_t) {
        .ops       = cs_ops,
        .handle    = cs_handle,
        .mult      = mult,
        .shift     = shift,
        .inv_mult  = inv_mult,
        .inv_shift = inv_shift,
    };
}

static inline clock_t* setup_new_clock(
    clock_t*        out,
    const device_t* clocksource,
    const device_t* timer,
    int64_t         offset,
    uint32_t        mult,
    uint32_t        shift,
    uint32_t        inv_mult,
    uint32_t        inv_shift,
    bool mutable)
{
    *out = (clock_t) {
        .dev_clocksource = clocksource,
        .dev_timer       = timer,
        .mult            = mult,
        .shift           = shift,
        .inv_mult        = inv_mult,
        .inv_shift       = inv_shift,
        .offset          = offset,
        .mutable         = mutable,
        .timer_lock      = SPINLOCK_INIT,
        .event_list      = NULL,
    };

    return out;
}


clock_t* clock_new(
    const device_t* clocksource,
    const device_t* timer,
    timepoint_t     current_time,
    bool mutable)
{
    clock_init_t init = init_clocksource_and_timer(clocksource, timer);

    uint64_t ticks = init.ops->get_tick(init.handle);

    int64_t offset = current_time - tick_to_ns(ticks, init.mult, init.shift);

    clock_t* c = kmalloc(sizeof(clock_t));

    return setup_new_clock(
        c,
        clocksource,
        timer,
        offset,
        init.mult,
        init.shift,
        init.inv_mult,
        init.inv_shift,
        mutable);
}


clock_t* clock_new_offset(
    const device_t* clocksource,
    const device_t* timer,
    duration_ns_t   offset,
    bool mutable)
{
    clock_init_t init = init_clocksource_and_timer(clocksource, timer);

    clock_t* c = kmalloc(sizeof(clock_t));

    return setup_new_clock(
        c,
        clocksource,
        timer,
        offset,
        init.mult,
        init.shift,
        init.inv_mult,
        init.inv_shift,
        mutable);
}


void clock_new_static(
    clock_t* new,
    const device_t* clocksource,
    const device_t* timer,
    duration_ns_t   offset,
    bool mutable)
{
    clock_init_t init = init_clocksource_and_timer(clocksource, timer);

    setup_new_clock(
        new,
        clocksource,
        timer,
        offset,
        init.mult,
        init.shift,
        init.inv_mult,
        init.inv_shift,
        mutable);
}


void clock_delete(clock_t* clock)
{
    kfree((void*)clock);
}

/// sets the current time, it will panic if the clock is immutable
void clock_set_time(clock_t* clock, timepoint_t current_time)
{
    ASSERT(
        clock->mutable,
        "clock_set_time: cannot set time on an immutable clock");
    clock->offset = current_time - clocksource_now_ns(clock);
}

timepoint_t clock_now(clock_t* clock)
{
    return clocksource_now_ns(clock) + clock->offset;
}

duration_ns_t clock_time_until(clock_t* clock, timepoint_t tp)
{
    return tp - clock_now(clock);
}
