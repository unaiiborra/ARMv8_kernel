#include "time.h"

#include <kernel/devices/device.h>
#include <kernel/init.h>
#include <kernel/panic.h>
#include <kernel/time.h>

#include "kernel/smp.h"
#include "lib/lock.h"

const char* STD_CLOCKSOURCE_NAMES[NUM_CPUS] = {
    "arm/generic-timer/clocksource_0",
    "arm/generic-timer/clocksource_1",
    "arm/generic-timer/clocksource_2",
    "arm/generic-timer/clocksource_3",
};

const char* STD_TIMER_NAMES[NUM_CPUS] = {
    "arm/generic-timer/timer_0",
    "arm/generic-timer/timer_1",
    "arm/generic-timer/timer_2",
    "arm/generic-timer/timer_3",
};


static clock_t HRT[NUM_CPUS];

clock_t* HRTIMER()
{
    return &HRT[get_cpuid()];
}

void time_ctrl_init_cpu()
{
    const device_t* clocksource = device_get_by_name(
        DEVICE_CLASS_CLOCKSOURCE,
        STD_CLOCKSOURCE_NAMES[get_cpuid()]);

    const device_t* timer = device_get_by_name(
        DEVICE_CLASS_TIMER,
        STD_TIMER_NAMES[get_cpuid()]);

    if (!clocksource)
        PANIC("time_ctrl: no hrt clocksource registered!");

    if (!timer)
        PANIC("time_ctrl: no hrt timer registered!");

    clock_new_static(HRTIMER(), clocksource, timer, 0, false);
}

static void ksleep_event(void* ctx)
{
    *((bool*)ctx) = true;
}

void ksleep(duration_ns_t ns)
{
    bool flag = false;
    timer_create_event_delta(HRTIMER(), ksleep_event, &flag, ns);

    sevl();
    while (!flag)
        wfi();
}
