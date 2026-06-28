#pragma once

#include <arm/sysregs/sysregs.h>
#include <kernel/time.h>
#include <lib/data_structures/kvec.h>
#include <stdint.h>

static constexpr uint64_t IMX8MP_CPU_FREQ = 1200000000;

#define clockcycle_reset()                                        \
    do {                                                          \
        asm volatile("isb" ::: "memory");                         \
        sysreg_write(PMCR_EL0, sysreg_read(PMCR_EL0) | (1 << 2)); \
    } while (0)

#define clockcycle_now(c)                                          \
    do {                                                           \
        _Static_assert(                                            \
            __builtin_types_compatible_p(__typeof__(c), uint64_t), \
            "clockcycle_now: 'c' must be uint64_t");               \
        asm volatile("isb\n"                                       \
                     "mrs %0, pmccntr_el0\n"                       \
                     : "=r"(c)::"memory");                         \
    } while (0)

#define wallclock_now(t)                                           \
    do {                                                           \
        _Static_assert(                                            \
            __builtin_types_compatible_p(__typeof__(t), uint64_t), \
            "wallclock_now: 't' must be uint64_t");                \
        asm volatile("isb\n"                                       \
                     "mrs %0, cntpct_el0\n"                        \
                     : "=r"(t)::"memory");                         \
    } while (0)

#define clockpoint_now(cycle, wall)                                    \
    do {                                                               \
        _Static_assert(                                                \
            __builtin_types_compatible_p(__typeof__(cycle), uint64_t), \
            "clockpoint_now: 'cycle' must be uint64_t");               \
        _Static_assert(                                                \
            __builtin_types_compatible_p(__typeof__(wall), uint64_t),  \
            "clockpoint_now: 'wall' must be uint64_t");                \
        asm volatile("isb\n"                                           \
                     "mrs %0, pmccntr_el0\n"                           \
                     "mrs %1, cntpct_el0\n"                            \
                     : "=r"(cycle), "=r"(wall)::"memory");             \
    } while (0)

typedef uint64_t clockcycle_t;

typedef struct {
    clockcycle_t cycle;
    uint64_t     wallclock;
} monitor_clockpoint_t;

typedef struct {
    clockcycle_t cycle_start;
    clockcycle_t cycle_end;
    uint64_t     cycle_delta;

    timepoint_t   wallclock_start;
    timepoint_t   wallclock_end;
    duration_ns_t wallclock_delta;

    uint64_t cpu_active_percentage; // 0/100
    uint64_t exception_count;
} monitor_data_t;

void performance_monitor_init();
void performance_monitor_fence();
// reads last fence
monitor_data_t performance_monitor_read_fence(uint64_t cpu_freq);

static inline monitor_clockpoint_t performance_monitor_now()
{
    uint64_t cycle_point, wallclock_point;
    clockpoint_now(cycle_point, wallclock_point);

    return (monitor_clockpoint_t) {
        .cycle     = cycle_point,
        .wallclock = wallclock_point,
    };
}

static inline monitor_data_t performance_monitor_data_build(
    monitor_clockpoint_t tstart,
    monitor_clockpoint_t tend,
    size_t               exceptions)
{
    uint64_t cntfrq      = sysreg_read(CNTFRQ_EL0);
    uint64_t cycle_delta = tend.cycle - tstart.cycle;
    uint64_t wall_delta  = tend.wallclock - tstart.wallclock;

    return (monitor_data_t) {
        .cycle_start           = tstart.cycle,
        .cycle_end             = tend.cycle,
        .cycle_delta           = cycle_delta,
        .wallclock_start       = tstart.wallclock,
        .wallclock_end         = tend.wallclock,
        .wallclock_delta       = wall_delta,
        .cpu_active_percentage = wall_delta ? ((__uint128_t)cycle_delta *
                                               cntfrq * 100) /
                                                  ((uint64_t)wall_delta *
                                                   IMX8MP_CPU_FREQ)
                                            : 0,
        .exception_count       = exceptions,
    };
}

monitor_data_t performance_monitor_print(uint64_t cpu_freq, const char* tag);
void           performance_monitor_print_metrics(
    const kvec(monitor_data_t) * monitor_data,
    uint64_t    cpu_freq,
    const char* tag);

void performance_monitor_fence_notify_exception();

timepoint_t clockpoint_to_timepoint(clockcycle_t clockcycle, uint64_t freq);
duration_ns_t clockpoint_rng_ns(clockcycle_t c0, clockcycle_t c1, uint64_t freq);
duration_ns_t clockcycle_print_profiling(
    clockcycle_t c0,
    clockcycle_t c1,
    uint64_t     freq,
    const char*  tag);
