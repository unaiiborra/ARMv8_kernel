#pragma once

#include <arm/sysregs/sysregs.h>
#include <kernel/time.h>
#include <stdint.h>

#include "lib/data_structures/kvec.h"

static constexpr uint64_t IMX8MP_CPU_FREQ = 1200000000;

typedef uint64_t clockcycle_t;

typedef struct {
    clockcycle_t cycle_start;
    clockcycle_t cycle_end;
    uint64_t     cycle_delta;

    clockcycle_t wallclock_start;
    clockcycle_t wallclock_end;
    uint64_t     wallclock_delta;

    uint64_t cpu_active_percentage;
    uint64_t exception_count;
} monitor_data_t;

void           performance_monitor_init();
void           performance_monitor_fence();
monitor_data_t performance_monitor_read(uint64_t cpu_freq);
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

#define clockcycle_reset()                                        \
    do {                                                          \
        asm volatile("isb" ::: "memory");                         \
        sysreg_write(PMCR_EL0, sysreg_read(PMCR_EL0) | (1 << 2)); \
    } while (0)

#define clockcycle_now(c)                    \
    do {                                     \
        asm volatile("isb\n"                 \
                     "mrs %0, pmccntr_el0\n" \
                     : "=r"(c)::"memory");   \
    } while (0)

#define wallclock_now(t)                    \
    do {                                    \
        asm volatile("isb\n"                \
                     "mrs %0, cntpct_el0\n" \
                     : "=r"(t)::"memory");  \
    } while (0)

#define clockpoint_now(cycle, wall)                        \
    do {                                                   \
        asm volatile("isb\n"                               \
                     "mrs %0, pmccntr_el0\n"               \
                     "mrs %1, cntpct_el0\n"                \
                     : "=r"(cycle), "=r"(wall)::"memory"); \
    } while (0)
