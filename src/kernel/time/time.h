#include <kernel/time.h>
#include <stdint.h>

#include "lib/lock.h"


typedef struct timer_node {
    struct timer_node *prev, *next;
    uint64_t           id;
    clock_t*           clock;
    // timepoint reltive to the clock (with offset already added)
    timepoint_t      expires;
    timer_callback_t event;
    void*            ctx;
} timer_node_t;


typedef struct clock {
    const device_t *dev_clocksource, *dev_timer;
    uint32_t        mult, shift, inv_mult, inv_shift;
    duration_ns_t   offset; // offset to epoch
    bool mutable;
    spinlock_t timer_lock;

    // used only if dev_timer != NULL
    timer_node_t* event_list;
} clock_t;


void clock_new_static(
    clock_t* new,
    const device_t* clocksource,
    const device_t* timer,
    duration_ns_t   offset,
    bool mutable);


static inline void compute_mult_shift(
    uint64_t  freq_hz,
    uint32_t* mult,
    uint32_t* shift,
    uint32_t* inv_mult,
    uint32_t* inv_shift)
{
    // ticks →  ns: mult/shift
    uint32_t s = 0;
    while (((UINT64_C(1000000000) << (s + 1)) / freq_hz) < (1ULL << 32))
        s++;
    *shift = s;
    *mult  = (UINT64_C(1000000000) << s) / freq_hz;

    // ns →  ticks: inv_mult/inv_shift
    uint32_t is = 0;
    while (((freq_hz << (is + 1)) / UINT64_C(1000000000)) < (1ULL << 32))
        is++;
    *inv_shift = is;
    *inv_mult  = (freq_hz << is) / UINT64_C(1000000000);
}


[[gnu::always_inline]] static inline int64_t tick_to_ns(
    uint64_t ticks,
    uint32_t mult,
    uint32_t shift)
{
    return (int64_t)(((__uint128_t)ticks * mult) >> shift);
}

[[gnu::always_inline]] static inline uint64_t ns_to_tick(
    int64_t  ns,
    uint32_t inv_mult,
    uint32_t inv_shift)
{
    return (uint64_t)(((__uint128_t)(uint64_t)ns * inv_mult) >> inv_shift);
}