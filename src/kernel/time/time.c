#include "time.h"

#include <kernel/devices/device.h>
#include <kernel/init.h>
#include <kernel/panic.h>
#include <kernel/time.h>

#include "lib/lock.h"


static clock_t HRT;

clock_t* const HRTIMER = &HRT;



void time_ctrl_init()
{
    // TODO: get it from other source, not only expect that both primary
    // clocksource and timer drivers are hardware related
    const device_t* clocksource = device_get_primary(DEVICE_CLASS_CLOCKSOURCE);
    const device_t* timer       = device_get_primary(DEVICE_CLASS_TIMER);

    if (!clocksource)
        PANIC("time_ctrl: no hrt clocksource registered!");

    if (!timer)
        PANIC("time_ctrl: no hrt timer registered!");

    clock_new_static(HRTIMER, clocksource, timer, 0, false);
}


static void ksleep_event(void* ctx)
{
    *((bool*)ctx) = true;
}

void ksleep(duration_ns_t ns)
{
    bool flag = false;
    timer_create_event_delta(HRTIMER, ksleep_event, &flag, ns);

    sevl();
    while (!flag)
        wfi();
}
