#include <kernel/io/stdio.h>
#include <lib/performance_monitor.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/hardware.h"
#include "kernel/mm.h"
#include "kernel/smp.h"
#include "kernel/time.h"
#include "lib/data_structures/kvec.h"
#include "lib/stdattribute.h"

static struct {
    _Alignas(CACHE_LINE) uint64_t exception_counter;
    uint64_t     exception_count;
    clockcycle_t cycle_start, cycle_end;
    uint64_t     wallclock_start, wallclock_end;
} fence_data[NUM_CPUS];

void performance_monitor_init()
{
    uint64_t val = sysreg_read(PMCR_EL0);
    val |= (1 << 0); // E: enable counters
    val |= (1 << 6); // LC: long cycle counter enable
    sysreg_write(PMCR_EL0, val);

    sysreg_write(PMCNTENSET_EL0, (1ULL << 31)); // enable cycle counter
    sysreg_write(PMCCFILTR_EL0, 0);             // disable filter

    for (cpuid_t cpuid = 0; cpuid < NUM_CPUS; cpuid++) {
        fence_data[cpuid].cycle_start       = 0;
        fence_data[cpuid].cycle_end         = 0;
        fence_data[cpuid].exception_count   = 0;
        fence_data[cpuid].exception_counter = 0;
    }
}

attr(gnu::optimize("O3")) void performance_monitor_fence()
{
    clockcycle_t cycle, wallclock;
    clockpoint_now(cycle, wallclock);
    cpuid_t cpuid = get_cpuid();

    fence_data[cpuid].wallclock_start   = fence_data[cpuid].wallclock_end;
    fence_data[cpuid].wallclock_end     = wallclock;
    fence_data[cpuid].cycle_start       = fence_data[cpuid].cycle_end;
    fence_data[cpuid].cycle_end         = cycle;
    fence_data[cpuid].exception_count   = fence_data[cpuid].exception_counter;
    fence_data[cpuid].exception_counter = 0;
    asm volatile("nop");
}

monitor_data_t performance_monitor_read_fence(uint64_t cpu_freq)
{
    uint64_t cntfrq = sysreg_read(CNTFRQ_EL0);

    cpuid_t  cpuid           = get_cpuid();
    uint64_t cycle_delta     = fence_data[cpuid].cycle_end -
                               fence_data[cpuid].cycle_start;
    uint64_t wallclock_delta = fence_data[cpuid].wallclock_end -
                               fence_data[cpuid].wallclock_start;

    return (monitor_data_t) {
        .cycle_start           = fence_data[cpuid].cycle_start,
        .cycle_end             = fence_data[cpuid].cycle_end,
        .cycle_delta           = cycle_delta,
        .wallclock_start       = fence_data[cpuid].wallclock_start,
        .wallclock_end         = fence_data[cpuid].wallclock_end,
        .wallclock_delta       = wallclock_delta,
        .cpu_active_percentage = cycle_delta * cntfrq * 100 /
                                 (wallclock_delta * cpu_freq),
        .exception_count       = fence_data[cpuid].exception_count,
    };
}

monitor_data_t performance_monitor_print(uint64_t cpu_freq, const char* tag)
{
    monitor_data_t data   = performance_monitor_read_fence(cpu_freq);
    uint64_t       cntfrq = sysreg_read(CNTFRQ_EL0);

    duration_ns_t wall_duration = clockpoint_rng_ns(
        data.wallclock_start,
        data.wallclock_end,
        cntfrq);
    uint64_t w_ns = wall_duration % 1000;
    uint64_t w_us = (wall_duration / 1000) % 1000;
    uint64_t w_ms = (wall_duration / 1000000) % 1000;
    uint64_t w_s  = (wall_duration / 1000000000);

    duration_ns_t cpu_duration = clockpoint_rng_ns(
        data.cycle_start,
        data.cycle_end,
        cpu_freq);
    uint64_t c_ns = cpu_duration % 1000;
    uint64_t c_us = (cpu_duration / 1000) % 1000;
    uint64_t c_ms = (cpu_duration / 1000000) % 1000;
    uint64_t c_s  = (cpu_duration / 1000000000);

    printf(
        "[CPU%l][%s]\n\r"
        "  wall:  %ls.%lms.%lus.%lns (ticks: %l-%l delta:%l)\n\r"
        "  cpu:   %ls.%lms.%lus.%lns (cycles: %l-%l delta:%l)\n\r"
        "  active: %l%% exceptions:%l\n\r",
        (uint64_t)get_cpuid(),
        tag,
        w_s,
        w_ms,
        w_us,
        w_ns,
        data.wallclock_start,
        data.wallclock_end,
        data.wallclock_delta,
        c_s,
        c_ms,
        c_us,
        c_ns,
        data.cycle_start,
        data.cycle_end,
        data.cycle_delta,
        data.cpu_active_percentage,
        data.exception_count);

    return data;
}

attr(gnu::optimize("O3")) void performance_monitor_print_metrics(
    const kvec(monitor_data_t) * monitor_data,
    uint64_t    cpu_freq,
    const char* tag)
{
    size_t          n      = kvec_len(monitor_data);
    monitor_data_t* data   = kvec_data(monitor_data);
    uint64_t        cntfrq = sysreg_read(CNTFRQ_EL0);

    if (n == 0) {
        printf("[%s] no samples\n\r", tag);
        return;
    }

    uint64_t wall_min = UINT64_MAX, wall_max = 0, wall_sum = 0;
    uint64_t cpu_min = UINT64_MAX, cpu_max = 0, cpu_sum = 0;
    uint64_t act_min = UINT64_MAX, act_max = 0, act_sum = 0;
    uint64_t exc_total = 0, exc_count = 0;
    uint64_t cyc_min = UINT64_MAX, cyc_max = 0, cyc_sum = 0;

    for (size_t i = 0; i < n; i++) {
        duration_ns_t wall = clockpoint_rng_ns(
            data[i].wallclock_start,
            data[i].wallclock_end,
            cntfrq);
        duration_ns_t cpu = clockpoint_rng_ns(
            data[i].cycle_start,
            data[i].cycle_end,
            cpu_freq);
        uint64_t act = data[i].wallclock_delta
                           ? ((__uint128_t)data[i].cycle_delta * cntfrq * 100) /
                                 ((uint64_t)data[i].wallclock_delta * cpu_freq)
                           : 0;
        uint64_t cyc = data[i].cycle_end - data[i].cycle_start;

        if ((uint64_t)wall < wall_min)
            wall_min = wall;
        if ((uint64_t)wall > wall_max)
            wall_max = wall;
        wall_sum += wall;

        if ((uint64_t)cpu < cpu_min)
            cpu_min = cpu;
        if ((uint64_t)cpu > cpu_max)
            cpu_max = cpu;
        cpu_sum += cpu;

        if (cyc < cyc_min)
            cyc_min = cyc;
        if (cyc > cyc_max)
            cyc_max = cyc;
        cyc_sum += cyc;

        if (act < act_min)
            act_min = act;
        if (act > act_max)
            act_max = act;
        act_sum += act;

        exc_total += data[i].exception_count;
        if (data[i].exception_count > 0)
            exc_count++;
    }

    uint64_t wall_avg = wall_sum / n;
    uint64_t cpu_avg  = cpu_sum / n;
    uint64_t act_avg  = act_sum / n;
    uint64_t cyc_avg  = cyc_sum / n;

    __uint128_t wall_var = 0, cpu_var = 0;
    uint64_t    jitter_min = UINT64_MAX, jitter_max = 0, jitter_sum = 0;

    for (size_t i = 0; i < n; i++) {
        duration_ns_t wall = clockpoint_rng_ns(
            data[i].wallclock_start,
            data[i].wallclock_end,
            cntfrq);
        duration_ns_t cpu = clockpoint_rng_ns(
            data[i].cycle_start,
            data[i].cycle_end,
            cpu_freq);

        int64_t dw = (int64_t)wall - (int64_t)wall_avg;
        int64_t dc = (int64_t)cpu - (int64_t)cpu_avg;
        wall_var += (__uint128_t)(dw < 0 ? -dw : dw) *
                    (uint64_t)(dw < 0 ? -dw : dw);
        cpu_var += (__uint128_t)(dc < 0 ? -dc : dc) *
                   (uint64_t)(dc < 0 ? -dc : dc);

        if (i > 0) {
            duration_ns_t prev_wall = clockpoint_rng_ns(
                data[i - 1].wallclock_start,
                data[i - 1].wallclock_end,
                cntfrq);
            uint64_t jitter = (uint64_t)wall > (uint64_t)prev_wall
                                  ? (uint64_t)wall - (uint64_t)prev_wall
                                  : (uint64_t)prev_wall - (uint64_t)wall;
            if (jitter < jitter_min)
                jitter_min = jitter;
            if (jitter > jitter_max)
                jitter_max = jitter;
            jitter_sum += jitter;
        }
    }

    /* varianza -> desviacion tipica con raiz entera (semilla por clz) */
    uint64_t wall_variance = (uint64_t)(wall_var / n);
    uint64_t cpu_variance  = (uint64_t)(cpu_var / n);

    uint64_t wall_std, cpu_std;
    if (wall_variance == 0) {
        wall_std = 0;
    }
    else {
        int      leading = __builtin_clzll(wall_variance);
        uint64_t x       = 1ULL << ((64 - leading) / 2 + 1);
        for (int i = 0; i < 16; i++)
            x = (x + wall_variance / x) / 2;
        wall_std = x;
    }
    if (cpu_variance == 0) {
        cpu_std = 0;
    }
    else {
        int      leading = __builtin_clzll(cpu_variance);
        uint64_t x       = 1ULL << ((64 - leading) / 2 + 1);
        for (int i = 0; i < 16; i++)
            x = (x + cpu_variance / x) / 2;
        cpu_std = x;
    }

    uint64_t jitter_avg = n > 1 ? jitter_sum / (n - 1) : 0;
    if (jitter_min == UINT64_MAX)
        jitter_min = 0;

    scoped_kfree_t wall_ptr    = kmalloc(sizeof(uint64_t) * n);
    scoped_kfree_t cpu_ptr     = kmalloc(sizeof(uint64_t) * n);
    uint64_t*      wall_sorted = wall_ptr;
    uint64_t*      cpu_sorted  = cpu_ptr;

    for (size_t i = 0; i < n; i++) {
        wall_sorted[i] = clockpoint_rng_ns(
            data[i].wallclock_start,
            data[i].wallclock_end,
            cntfrq);
        cpu_sorted[i] = clockpoint_rng_ns(
            data[i].cycle_start,
            data[i].cycle_end,
            cpu_freq);
    }

    for (size_t i = 1; i < n; i++) {
        uint64_t key = wall_sorted[i];
        size_t   j   = i;
        while (j > 0 && wall_sorted[j - 1] > key) {
            wall_sorted[j] = wall_sorted[j - 1];
            j--;
        }
        wall_sorted[j] = key;
    }

    for (size_t i = 1; i < n; i++) {
        uint64_t key = cpu_sorted[i];
        size_t   j   = i;
        while (j > 0 && cpu_sorted[j - 1] > key) {
            cpu_sorted[j] = cpu_sorted[j - 1];
            j--;
        }
        cpu_sorted[j] = key;
    }

#define PCTILE(arr, n, p) (arr[((n) * (p)) / 100])
    uint64_t wall_p25 = PCTILE(wall_sorted, n, 25);
    uint64_t wall_p50 = PCTILE(wall_sorted, n, 50);
    uint64_t wall_p75 = PCTILE(wall_sorted, n, 75);
    uint64_t wall_p90 = PCTILE(wall_sorted, n, 90);
    uint64_t wall_p95 = PCTILE(wall_sorted, n, 95);
    uint64_t wall_p99 = PCTILE(wall_sorted, n, 99);
    uint64_t cpu_p25  = PCTILE(cpu_sorted, n, 25);
    uint64_t cpu_p50  = PCTILE(cpu_sorted, n, 50);
    uint64_t cpu_p75  = PCTILE(cpu_sorted, n, 75);
    uint64_t cpu_p90  = PCTILE(cpu_sorted, n, 90);
    uint64_t cpu_p95  = PCTILE(cpu_sorted, n, 95);
    uint64_t cpu_p99  = PCTILE(cpu_sorted, n, 99);
#undef PCTILE

    uint64_t wall_iqr = wall_p75 - wall_p25;
    uint64_t cpu_iqr  = cpu_p75 - cpu_p25;

#define FMT_NS(v) \
    (v) / 1000000000, ((v) / 1000000) % 1000, ((v) / 1000) % 1000, (v) % 1000

    printf(
        "[CPU%l][%s] N=%l\n\r"
        "  wall   min=%ls.%lms.%lus.%lns\n\r"
        "         max=%ls.%lms.%lus.%lns\n\r"
        "         avg=%ls.%lms.%lus.%lns\n\r"
        "         std=%ls.%lms.%lus.%lns\n\r"
        "         iqr=%ls.%lms.%lus.%lns\n\r"
        "         p25=%ls.%lms.%lus.%lns\n\r"
        "         p50=%ls.%lms.%lus.%lns\n\r"
        "         p75=%ls.%lms.%lus.%lns\n\r"
        "         p90=%ls.%lms.%lus.%lns\n\r"
        "         p95=%ls.%lms.%lus.%lns\n\r"
        "         p99=%ls.%lms.%lus.%lns\n\r"
        "  cpu    min=%ls.%lms.%lus.%lns\n\r"
        "         max=%ls.%lms.%lus.%lns\n\r"
        "         avg=%ls.%lms.%lus.%lns\n\r"
        "         std=%ls.%lms.%lus.%lns\n\r"
        "         iqr=%ls.%lms.%lus.%lns\n\r"
        "         p25=%ls.%lms.%lus.%lns\n\r"
        "         p50=%ls.%lms.%lus.%lns\n\r"
        "         p75=%ls.%lms.%lus.%lns\n\r"
        "         p90=%ls.%lms.%lus.%lns\n\r"
        "         p95=%ls.%lms.%lus.%lns\n\r"
        "         p99=%ls.%lms.%lus.%lns\n\r"
        "  cycles min=%l max=%l avg=%l\n\r"
        "  active min=%l%% max=%l%% avg=%l%%\n\r"
        "  jitter min=%ls.%lms.%lus.%lns\n\r"
        "         max=%ls.%lms.%lus.%lns\n\r"
        "         avg=%ls.%lms.%lus.%lns\n\r"
        "  exceptions total=%l\n\r"
        "  samples_with_exceptions=%l/%l\n\r",
        (uint64_t)get_cpuid(),
        tag,
        (uint64_t)n,
        FMT_NS(wall_min),
        FMT_NS(wall_max),
        FMT_NS(wall_avg),
        FMT_NS(wall_std),
        FMT_NS(wall_iqr),
        FMT_NS(wall_p25),
        FMT_NS(wall_p50),
        FMT_NS(wall_p75),
        FMT_NS(wall_p90),
        FMT_NS(wall_p95),
        FMT_NS(wall_p99),
        FMT_NS(cpu_min),
        FMT_NS(cpu_max),
        FMT_NS(cpu_avg),
        FMT_NS(cpu_std),
        FMT_NS(cpu_iqr),
        FMT_NS(cpu_p25),
        FMT_NS(cpu_p50),
        FMT_NS(cpu_p75),
        FMT_NS(cpu_p90),
        FMT_NS(cpu_p95),
        FMT_NS(cpu_p99),
        cyc_min,
        cyc_max,
        cyc_avg,
        act_min,
        act_max,
        act_avg,
        FMT_NS(jitter_min),
        FMT_NS(jitter_max),
        FMT_NS(jitter_avg),
        exc_total,
        exc_count,
        (uint64_t)n);

#undef FMT_NS
}

attr(gnu::optimize("O3")) void performance_monitor_fence_notify_exception()
{
    fence_data[get_cpuid()].exception_counter++;
}


timepoint_t clockpoint_to_timepoint(clockcycle_t clockcycle, uint64_t freq)
{
    return (timepoint_t)((__uint128_t)clockcycle * 1000000000ULL / freq);
}

duration_ns_t clockpoint_rng_ns(clockcycle_t c0, clockcycle_t c1, uint64_t freq)
{
    return (duration_ns_t)((__int128_t)((int64_t)c1 - (int64_t)c0) *
                           1000000000 / freq);
}

duration_ns_t clockcycle_print_profiling(
    clockcycle_t c0,
    clockcycle_t c1,
    uint64_t     freq,
    const char*  tag)
{
    duration_ns_t duration = clockpoint_rng_ns(c0, c1, freq);

    uint64_t ns = duration % 1000;
    uint64_t us = (duration / 1000) % 1000;
    uint64_t ms = (duration / 1000000) % 1000;
    uint64_t s  = (duration / 1000000000);

    printf(
        "[%s] %ls.%lms.%lus.%lns (cycles: %l-%l delta:%l)\n\r",
        tag,
        s,
        ms,
        us,
        ns,
        c0,
        c1,
        c1 - c0);

    return duration;
}
