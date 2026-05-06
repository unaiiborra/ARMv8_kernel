#include <arm/sysregs/sysregs.h>
#include <drivers/arm_generic_timer.h>
#include <kernel/devices/device.h>
#include <kernel/devices/driver_ops/clocksource.h>
#include <lib/lock.h>
#include <lib/stdmacros.h>
#include <stddef.h>
#include <stdint.h>



/*   CLOCKSOURCE   */

static uint64_t clocksource_get_tick([[maybe_unused]] driver_handle_t handle)
{
    return sysreg_read(CNTPCT_EL0);
}

static uint64_t clocksource_get_freq_hz([[maybe_unused]] driver_handle_t handle)
{
    return sysreg_read(CNTFRQ_EL0);
}


static const clocksource_ops_t CLOCKSOURCE_OPS = {
    .init        = NULL,
    .exit        = NULL,
    .get_tick    = clocksource_get_tick,
    .get_freq_hz = clocksource_get_freq_hz,
    .irq_enable  = NULL,
    .irq_disable = NULL,
    .irq_handle  = NULL,
};

const clocksource_ops_t* const
    ARM_GENERIC_TIMER_CLOCKSOURCE_OPS = &CLOCKSOURCE_OPS;




/*   TIMER   */

typedef struct {
    void (*handler)(void* ctx);
    void* ctx;
    bool  running;
} agt_timer_state_t;


#define ISTATUS(v) ((uint64_t)(!!v) << 2)
#define IMASK(v)   ((uint64_t)(!!v) << 1)
#define IENABLE(v) ((uint64_t)(!!v))


static int32_t timer_init(driver_handle_t handle)
{
    if (*handle.state != NULL)
        return -1;

    irqlocked()
    {
        agt_timer_state_t* state = kmalloc(sizeof(agt_timer_state_t));

        state->handler = NULL;
        state->ctx     = NULL;
        state->running = false;

        *handle.state = state;

        sysreg_write(CNTV_CTL_EL0, IMASK(false) | IENABLE(false));
    }

    return 0;
}


static int32_t timer_exit(driver_handle_t handle)
{
    if (*handle.state == NULL)
        return -1;

    irqlocked()
    {
        sysreg_write(CNTV_CTL_EL0, IMASK(true) | IENABLE(false));

        kfree(*handle.state);
        *handle.state = NULL;
    }

    return 0;
}


static int32_t timer_irq_notify_tick(
    driver_handle_t handle,
    uint64_t tick, // the clocksource device (for the frequency) of the tick
                   // must be stablshed at a higher abstraction level
    void (*handler)(void* ctx), // NULL for stop
    void* ctx                   // ignored if handler == NULL
)
{
    if (unlikely(*handle.state == NULL))
        return -1;


    agt_timer_state_t* state = *handle.state;

    if (handler == NULL) {
        irqlock_t l = irq_lock();

        sysreg_write(CNTV_CTL_EL0, IMASK(false) | IENABLE(false));

        state->handler = NULL;
        state->ctx     = NULL;
        state->running = false;

        irq_unlock(l);

        return 0;
    }

    irqlock_t l = irq_lock();

    state->handler = handler;
    state->ctx     = ctx;
    state->running = true;

    sysreg_write(CNTV_CVAL_EL0, tick);
    sysreg_write(CNTV_CTL_EL0, IMASK(false) | IENABLE(true));

    irq_unlock(l);

    return 0;
}

static int32_t timer_is_running([[maybe_unused]] driver_handle_t handle)
{
    return sysreg_read(CNTV_CTL_EL0) & IENABLE(true);
}

static void timer_irq_handle(driver_handle_t handle)
{
    irqlock_t l = irq_lock();

    agt_timer_state_t* state = *handle.state;

    DEBUG_ASSERT(
        (sysreg_read(CNTV_CTL_EL0) &
         (ISTATUS(true) | IMASK(false) | IENABLE(true))) ==
        (ISTATUS(true) | IMASK(false) | IENABLE(true)));

    sysreg_write(
        CNTV_CTL_EL0,
        IMASK(true) | IENABLE(true)); // mask to avoid reentrance

    if (state->handler != NULL)
        state->handler(state->ctx);

    uint64_t ctl = sysreg_read(CNTV_CTL_EL0);

    if (ctl & IMASK(true)) {
        sysreg_write(CNTV_CTL_EL0, IMASK(false) | IENABLE(false));
        state->running = false;
    }

    irq_unlock(l);
}




static const timer_ops_t TIMER_OPS = {
    .init            = timer_init,
    .exit            = timer_exit,
    .irq_enable      = NULL,
    .irq_disable     = NULL,
    .irq_notify_tick = timer_irq_notify_tick,
    .is_running      = timer_is_running,
    .irq_handle      = timer_irq_handle,
};

const timer_ops_t* const ARM_GENERIC_TIMER_OPS = &TIMER_OPS;
