#include <arm/sysregs/sysregs.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <lib/lock/spinlock.h>
#include <lib/lock/spinlock_irq.h>
#include <lib/stdmacros.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/devices/device.h"
#include "lib/lock/_lock_types.h"

#define cycles() sysreg_read(CNTVCT_EL0)
#define freq() sysreg_read(CNTFRQ_EL0)

uint64_t AGT_ns_to_cycles(uint64_t ns)
{
    uint64_t f = freq();
    uint64_t sec = ns / 1000000000ULL;
    uint64_t rem = ns % 1000000000ULL;

    return sec * f + (rem * f) / 1000000000ULL;
}

uint64_t AGT_us_to_cycles(uint64_t us)
{
    uint64_t f = freq();
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
#define enable_timer() sysreg_write(CNTV_CTL_EL0, 1)
#define disable_timer() sysreg_write(CNTV_CTL_EL0, 0)

// ISTATUS (bit 2)
#define timer_fired() ((bool)((_ARM_CNTV_CTL_EL0_get() >> 2) & 0b1ul))

// Unlocked AGT_timer_schedule_cb
static inline bool UNLOCKED_timer_schedule_(
    const driver_handle* h,
    uint64_t cycles,
    timer_cb_t cb,
    timer_arg arg)
{
    bool overrides;
    agt_state* state = get_state_(h);

    overrides = timer_is_enabled();

    disable_timer();

    state->timer_cb = cb;
    state->arg = arg;
    state->timer_fired = false;


    sysreg_write(CNTV_CVAL_EL0, cycles);

    enable_timer();

    return overrides;
}

bool AGT_timer_schedule_cycles(
    const driver_handle* h,
    uint64_t cycles,
    timer_cb_t cb,
    timer_arg arg)
{
    agt_state* state = get_state_(h);
    bool overrides;

    // check if this call has been made under another timer callback, if true,
    // defer the new scheduling
    if (spin_try_lock(&state->defer_cb.under_callback_gate)) {
        // set after callback values
        state->timer_fired = false;
        state->defer_cb.under_cb_timer_cb = cb;
        state->defer_cb.under_cb_arg = arg;
        state->defer_cb.cycles_v = cycles;

        // after the callback irqs will be enabled
        overrides = state->defer_cb.under_cb_scheduled;
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
    uint64_t delta_ns,
    timer_cb_t cb,
    timer_arg arg)
{
    agt_state* state = get_state_(h);
    bool overrides;

    uint64_t cycles = cycles() + AGT_ns_to_cycles(delta_ns);

    // check if this call has been made under another timer callback, if true,
    // defer the new scheduling
    if (spin_try_lock(&state->defer_cb.under_callback_gate)) {
        // set after callback values
        state->timer_fired = false;
        state->defer_cb.under_cb_timer_cb = cb;
        state->defer_cb.under_cb_arg = arg;

        // set the value as delta ns
        state->defer_cb.cycles_v = cycles;

        // after the callback irqs will be enabled
        overrides = state->defer_cb.under_cb_scheduled;
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
    bool fired;
    agt_state* state = get_state_(h);

    if (spin_try_lock(&state->defer_cb.under_callback_gate)) {
        fired = state->timer_fired; // it is safe to access this value without
                                    // using the main lock because if under a
                                    // callback, the lock is already taken
        state->timer_fired = false;
        spin_unlock(&state->defer_cb.under_callback_gate);
        return fired;
    }

    irqlock_t irqf = _spin_lock_irqsave(&state->lock);
    fired = state->timer_fired;
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

        state->timer_fired = true;
        state->defer_cb.under_cb_timer_cb = NULL;
        state->defer_cb.under_cb_arg = NULL;
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

void AGT_init_stage0(const driver_handle* h)
{
    agt_state* state = get_state_(h);

    state->lock.slock = 0; // open
    state->timer_cb = NULL;
    state->arg = NULL;
    state->timer_fired = false;
    // locked (starts locked as it opens when it is under a callback)
    state->defer_cb.under_callback_gate.slock = 1;
    state->defer_cb.under_cb_scheduled = false;
    state->defer_cb.under_cb_timer_cb = NULL;
    state->defer_cb.under_cb_arg = NULL;
    state->defer_cb.cycles_v = 0;
}
