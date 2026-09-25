#pragma once

#include "kernel/smp.h"
#include <kernel/devices/device.h>
#include <stdint.h>

typedef int64_t timepoint_t;
typedef int64_t duration_ns_t;

/* Clock (includes a clocksource and supports a related timer) */

typedef struct kclock kclock_t;

extern const char *STD_CLOCKSOURCE_NAMES[NUM_CPUS];
extern const char *STD_TIMER_NAMES[NUM_CPUS];

kclock_t *hrtimer_get(cpuid_t cpuid);
kclock_t *hrtimer_get_cpu_local();

void kclock_init_cpu();

/*
   kclock is an abstraction that allows both setting up timers and getting
   timepoints. It requires a clocksource and a timer driver that are related in
   frequency and ticks to work (hardware related). If a NULL timer driver is
   provided, it will only allow getting timepoints. The clocksource cannot be
   NULL. Using the driver ops directly when they are registered to a clock can
   cause ub as the clock might have set timer irq events, so it should be
   handled directly from the clock. It allows for virtual timer events (multiple
   events creation even if only one event is allowed from hardware side)
*/
kclock_t *kclock_new(
	const device_t *clocksource,
	const device_t *timer,
	timepoint_t current_time,
	bool mutable
);

kclock_t *kclock_new_offset(
	const device_t *clocksource,
	const device_t *timer,
	duration_ns_t offset /*
			      * offset from the clocksource to the current time,
			      * zero to mantain exactly the clocksource time
			      */
	,
	bool mutable
);

void kclock_delete(kclock_t *clock);

/// sets the current time, it will panic if the clock is immutable
void kclock_set_time(kclock_t *clock, timepoint_t current_time);
timepoint_t kclock_now(kclock_t *clock);
duration_ns_t kclock_time_until(kclock_t *clock, timepoint_t tp);

static inline duration_ns_t get_duration_ns(timepoint_t t0, timepoint_t t1)
{
	return (duration_ns_t)(t1 - t0);
}

#define __TIME_CONCAT(a, b)  a##b
#define __TIME_CONCAT2(a, b) __TIME_CONCAT(a, b)

#define kkclock_measure_impl(clock, duration_ptr, ctr)                                             \
	for (timepoint_t __TIME_CONCAT2(_tp_, ctr) = clock_now((clock)),                           \
					      __TIME_CONCAT2(_iter_, ctr) = 0;                     \
	     __TIME_CONCAT2(_iter_, ctr) < 1;                                                      \
	     *(duration_ptr) = clock_now(clock) - __TIME_CONCAT2(_tp_, ctr),                       \
					      __TIME_CONCAT2(_iter_, ctr)++)

#define kclock_measure(clock, duration_ptr) kkclock_measure_impl(clock, duration_ptr, __COUNTER__)

/* Core local timer */

typedef void (*timer_callback_t)(void *ctx);

typedef struct {
	uint64_t event_id;
	timepoint_t expiration_point;
	kclock_t *clock;
} timer_event_t;

timer_event_t timer_create_event(kclock_t *clock, timer_callback_t cb, void *ctx, timepoint_t t);

timer_event_t
timer_create_event_delta(kclock_t *clock, timer_callback_t cb, void *ctx, duration_ns_t delta_ns);

bool timer_cancel_event(timer_event_t event);

void ksleep(duration_ns_t ns);
