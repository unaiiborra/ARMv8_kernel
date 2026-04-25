#include <arm/sysregs/sysregs.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <kernel/devices/device.h>
#include <lib/lock.h>
#include <lib/stdmacros.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/branch.h"


#define cycles() sysreg_read(CNTVCT_EL0)
#define freq()   sysreg_read(CNTFRQ_EL0)

uint64_t AGT_ns_to_cycles(uint64_t ns)
{
    uint64_t f   = freq();
    uint64_t sec = ns / 1000000000ULL;
    uint64_t rem = ns % 1000000000ULL;

    return sec * f + (rem * f) / 1000000000ULL;
}

uint64_t AGT_us_to_cycles(uint64_t us)
{
    uint64_t f   = freq();
    uint64_t sec = us / 1000000ULL;
    uint64_t rem = us % 1000000ULL;

    return sec * f + (rem * f) / 1000000ULL;
}

uint64_t AGT_cnt_cycles(void)
{
    return cycles();
}

uint64_t AGT_cnt_freq(void)
{
    return freq();
}

uint64_t AGT_cnt_time_ns(void)
{
    __uint128_t c = cycles();
    __uint128_t f = freq();

    return (uint64_t)(c * 1000000000 / f);
}

uint64_t AGT_cnt_time_us(void)
{
    __uint128_t c = cycles();
    __uint128_t f = freq();

    return (uint64_t)(c * 1000000 / f);
}

/*
 *  Timer
 */
static inline agt_state* get_state_(const driver_handle* h)
{
    return (agt_state*)h->state;
}


// ENABLE (bit 0)
#define timer_is_enabled() (bool)(sysreg_read(CNTV_CTL_EL0) & 0b1ul)
#define enable_timer()     sysreg_write(CNTV_CTL_EL0, 1)
#define disable_timer()    sysreg_write(CNTV_CTL_EL0, 0)

// ISTATUS (bit 2)
#define timer_fired() ((bool)((sysreg_read(CNTV_CTL_EL0) >> 2) & 0b1ul))

// Unlocked AGT_timer_schedule_cb
static inline bool UNLOCKED_timer_schedule_(
    const driver_handle* h,
    uint64_t             cycles,
    timer_cb_t           cb,
    timer_arg            arg)
{
    bool       overrides;
    agt_state* state = get_state_(h);

    overrides = timer_is_enabled();

    disable_timer();

    state->timer_cb    = cb;
    state->arg         = arg;
    state->timer_fired = false;


    sysreg_write(CNTV_CVAL_EL0, cycles);

    enable_timer();

    return overrides;
}

bool AGT_timer_schedule_cycles(
    const driver_handle* h,
    uint64_t             cycles,
    timer_cb_t           cb,
    timer_arg            arg)
{
    agt_state* state     = get_state_(h);
    bool       overrides = false;

    // check if this call has been made under another timer callback, if true,
    // defer the new scheduling
    if (spin_try_lock(&state->defer_cb.under_callback_gate)) {
        // set after callback values
        state->timer_fired                = false;
        state->defer_cb.under_cb_timer_cb = cb;
        state->defer_cb.under_cb_arg      = arg;
        state->defer_cb.cycles_v          = cycles;

        // after the callback irqs will be enabled
        overrides                          = state->defer_cb.under_cb_scheduled;
        state->defer_cb.under_cb_scheduled = true;

        spin_unlock(&state->defer_cb.under_callback_gate);
        return overrides;
    }

    // standard function
    irq_spinlocked(&state->lock)
    {
        overrides = UNLOCKED_timer_schedule_(h, cycles, cb, arg);
    }

    return overrides;
}

bool AGT_timer_schedule_delta(
    const driver_handle* h,
    uint64_t             delta_ns,
    timer_cb_t           cb,
    timer_arg            arg)
{
    agt_state* state     = get_state_(h);
    bool       overrides = false;

    uint64_t cycles = cycles() + AGT_ns_to_cycles(delta_ns);

    // check if this call has been made under another timer callback, if true,
    // defer the new scheduling
    if (spin_try_lock(&state->defer_cb.under_callback_gate)) {
        // set after callback values
        state->timer_fired                = false;
        state->defer_cb.under_cb_timer_cb = cb;
        state->defer_cb.under_cb_arg      = arg;

        // set the value as delta ns
        state->defer_cb.cycles_v = cycles;

        // after the callback irqs will be enabled
        overrides                          = state->defer_cb.under_cb_scheduled;
        state->defer_cb.under_cb_scheduled = true;

        spin_unlock(&state->defer_cb.under_callback_gate);
        return overrides;
    }

    // standard function
    irq_spinlocked(&state->lock)
    {
        overrides = UNLOCKED_timer_schedule_(h, cycles, cb, arg);
    }

    return overrides;
}

bool AGT_timer_is_armed(const driver_handle* h)
{
    agt_state* state = get_state_(h);

    if (spin_try_lock(&state->defer_cb.under_callback_gate))
        return state->defer_cb.under_cb_scheduled;

    return timer_is_enabled();
}

bool AGT_timer_has_fired(const driver_handle* h)
{
    bool       fired;
    agt_state* state = get_state_(h);

    if (spin_try_lock(&state->defer_cb.under_callback_gate)) {
        fired = state->timer_fired; // it is safe to access this value without
                                    // using the main lock because if under a
                                    // callback, the lock is already taken
        state->timer_fired = false;
        spin_unlock(&state->defer_cb.under_callback_gate);
        return fired;
    }

    irqlock_t irqf     = _spin_lock_irqsave(&state->lock);
    fired              = state->timer_fired;
    state->timer_fired = false;
    _spin_unlock_irqrestore(&state->lock, irqf);

    return fired;
}

void AGT_timer_cancel(const driver_handle* h)
{
    agt_state* state = get_state_(h);

    if (spin_try_lock(&state->defer_cb.under_callback_gate)) {
        // Cancels the defered timer
        state->defer_cb.under_cb_scheduled = false;
        spin_unlock(&state->defer_cb.under_callback_gate);
        return;
    }

    disable_timer();
};

void AGT_handle_irq(const driver_handle* h)
{
    agt_state* state = get_state_(h);

    irq_spinlocked(&state->lock)
    {
        disable_timer();

        state->timer_fired                 = true;
        state->defer_cb.under_cb_timer_cb  = NULL;
        state->defer_cb.under_cb_arg       = NULL;
        state->defer_cb.under_cb_scheduled = false;

        if (state->timer_cb != NULL) {
            // unlocks if inside the cb, else locked
            spin_unlock(&state->defer_cb.under_callback_gate);
            state->timer_cb(state->arg);
            spin_lock(&state->defer_cb.under_callback_gate);
        }

        // If a new schedule was set while under the callback,
        // under_cb_scheduled will be true. This is the defer of the scheduling.
        if (state->defer_cb.under_cb_scheduled)
            UNLOCKED_timer_schedule_(
                h,
                state->defer_cb.cycles_v,
                state->defer_cb.under_cb_timer_cb,
                state->defer_cb.under_cb_arg);
    }
}

void AGT_init(const driver_handle* h)
{
    agt_state* state = get_state_(h);

    state->lock.slock  = 0; // open
    state->timer_cb    = NULL;
    state->arg         = NULL;
    state->timer_fired = false;
    // locked (starts locked as it opens when it is under a callback)
    state->defer_cb.under_callback_gate.slock = 1;
    state->defer_cb.under_cb_scheduled        = false;
    state->defer_cb.under_cb_timer_cb         = NULL;
    state->defer_cb.under_cb_arg              = NULL;
    state->defer_cb.cycles_v                  = 0;
}


/*   CLOCKSOURCE   */

static uint64_t clocksource_get_ticks([[maybe_unused]] driver_handle_t handle)
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
    .get_ticks   = clocksource_get_ticks,
    .get_freq_hz = clocksource_get_freq_hz,
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
    uint64_t        tick, // the clocksource of the tick must be stablshed at a
                          // higher abstraction level
    void (*handler)(void* ctx), // NULL for stop
    void* ctx)
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
    return timer_is_enabled();
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
