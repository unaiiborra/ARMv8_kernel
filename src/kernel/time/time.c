#include "time.h"

#include <kernel/devices/device.h>
#include <kernel/init.h>
#include <kernel/panic.h>
#include <kernel/time.h>

#include "kernel/smp.h"
#include "lib/lock.h"

const char *STD_CLOCKSOURCE_NAMES[NUM_CPUS] = {
	"arm/generic-timer/clocksource_0",
	"arm/generic-timer/clocksource_1",
	"arm/generic-timer/clocksource_2",
	"arm/generic-timer/clocksource_3",
};

const char *STD_TIMER_NAMES[NUM_CPUS] = {
	"arm/generic-timer/timer_0",
	"arm/generic-timer/timer_1",
	"arm/generic-timer/timer_2",
	"arm/generic-timer/timer_3",
};

static kclock_t HRT[NUM_CPUS];

kclock_t *hrtimer_get(cpuid_t cpuid)
{
	ASSERT(cpuid < NUM_CPUS);
	return &HRT[cpuid];
}

kclock_t *hrtimer_get_cpu_local()
{
	return hrtimer_get(get_cpuid());
}

void kclock_init_cpu()
{
	const device_t *clocksource =
		device_get_by_name(DEVICE_CLASS_CLOCKSOURCE, STD_CLOCKSOURCE_NAMES[get_cpuid()]);

	const device_t *timer =
		device_get_by_name(DEVICE_CLASS_TIMER, STD_TIMER_NAMES[get_cpuid()]);

	if (!clocksource) {
		PANIC("time_ctrl: no hrt clocksource registered!");
	}

	if (!timer) {
		PANIC("time_ctrl: no hrt timer registered!");
	}

	clock_new_static(hrtimer_get_cpu_local(), clocksource, timer, 0, false);
}

static void ksleep_event(void *ctx)
{
	*((volatile bool *)ctx) = true;
}

void ksleep(duration_ns_t ns)
{
	volatile bool flag = false;
	timer_create_event_delta(hrtimer_get_cpu_local(), ksleep_event, (void *)&flag, ns);

	sevl();
	while (!flag) {
		wfi();
	}
}
