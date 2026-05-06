#include <kernel/devices/device.h>
#include <kernel/devices/driver_ops/timer.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/time.h>
#include <lib/branch.h>
#include <lib/lock.h>
#include <lib/mem.h>
#include <lib/stdattribute.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/io/stdio.h"
#include "time.h"


static atomic_ulong timer_event_uid_counter = 0;

static void handle_events(void* clock);


/// setup the timer for the closest expiration time
static inline void setup_head_timer(clock_t* clock)
{
    const timer_ops_t* t_ops = get_timer_ops(clock->dev_timer);

    duration_ns_t abs_timer_ns = clock->event_list->expires -
                                 clock->offset; // get ns relative to the device

    uint64_t dev_tick = ns_to_tick(
        abs_timer_ns,
        clock->inv_mult,
        clock->inv_shift);

    int32_t op_res = t_ops->irq_notify_tick(
        device_get_driver_handle(clock->dev_timer),
        dev_tick,
        handle_events,
        clock);

    ASSERT(op_res >= 0);
}

static void handle_events(void* clock)
{
    defer(kvec_delete) kvec(timer_node_t*) events = kvec_new(timer_node_t*);

    clock_t* clk = clock;

    irq_spinlocked(&clk->timer_lock)
    {
        timer_node_t* cur = clk->event_list;

        while (cur && clock_now(cur->clock) >= cur->expires) {
            kvec_push(&events, &cur);

            cur = cur->next;
        }

        clk->event_list = cur;

        if (likely(cur)) {
            cur->prev = NULL;

            setup_head_timer(clk);
        }
    }

    const size_t   N              = kvec_len(&events);
    timer_node_t** expired_events = kvec_dataT(timer_node_t*, &events);

    for (size_t i = 0; i < N; i++) {
        timer_callback_t event = expired_events[i]->event;
        void*            ctx   = expired_events[i]->ctx;

        event(ctx);
    }

    // free everything after the callbacks to improve the timing precission and
    // avoid the overhead of kfree
    for (size_t i = 0; i < N; i++)
        kfree(expired_events[i]);
}


timer_event_t timer_create_event(
    clock_t*         clock,
    timer_callback_t cb,
    void*            ctx,
    timepoint_t      t)
{
    if (unlikely(clock->dev_timer == NULL))
        PANIC("timer: provided clock does not support timer operations");

    timer_node_t* node = kmalloc(sizeof(timer_node_t));

    uint64_t event_id = atomic_fetch_add(&timer_event_uid_counter, 1);

    *node = (timer_node_t) {
        .prev    = NULL,
        .next    = NULL,
        .id      = event_id,
        .clock   = clock,
        .expires = t,
        .event   = cb,
        .ctx     = ctx,
    };


    irq_spinlocked(&clock->timer_lock)
    {
        if (clock->event_list) {
            timer_node_t* curr = clock->event_list;

            while (curr && curr->expires <= t) {
                curr = curr->next;
            }

            if (curr) {
                node->next = curr;
                node->prev = curr->prev;

                if (curr->prev)
                    curr->prev->next = node;
                else
                    clock->event_list = node;

                curr->prev = node;
            }
            else {
                timer_node_t* tail = clock->event_list;
                while (tail->next)
                    tail = tail->next;

                tail->next = node;
                node->prev = tail;
            }
        }
        else {
            clock->event_list = node;
        }
    }

    if (clock->event_list == node /* is head */) {
        setup_head_timer(clock);
    }

    return (timer_event_t) {
        .event_id         = event_id,
        .expiration_point = t,
        .clock            = clock,
    };
}


timer_event_t timer_create_event_delta(
    clock_t*         clock,
    timer_callback_t cb,
    void*            ctx,
    duration_ns_t    delta_ns)
{
    return timer_create_event(clock, cb, ctx, clock_now(clock) + delta_ns);
}


bool timer_cancel_event(timer_event_t event)
{
    clock_t* clock = event.clock;

    irq_spinlocked(&clock->timer_lock)
    {
        timer_node_t* curr = clock->event_list;

        while (curr && curr->id != event.event_id) {
            curr = curr->next;
        }

        timer_node_t* prev = curr->prev;
        timer_node_t* next = curr->next;

        if (prev)
            prev->next = next;
        else
            clock->event_list = next;

        if (next)
            next->prev = prev;

        bool removed_head = (prev == NULL);

        kfree(curr);

        if (removed_head && clock->event_list)
            setup_head_timer(clock);
    }

    dbg_printf(DEBUG_TRACE, "timer: canceled event %d", event.event_id);

    return true;
}