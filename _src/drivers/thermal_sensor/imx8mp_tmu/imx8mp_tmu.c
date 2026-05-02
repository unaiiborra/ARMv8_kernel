#include <drivers/imx8mp_tmu.h>
#include <kernel/panic.h>
#include <lib/lock.h>
#include <lib/stdmacros.h>

#include "tasr.h"
#include "tcaliv0.h"
#include "tcaliv1.h"
#include "tcaliv_m40.h"
#include "ter.h"
#include "tidr.h"
#include "tier.h"
#include "tmhtactr.h"
#include "tmhtatr.h"
#include "tmhtitr.h"
#include "tmu_map.h"
#include "tps.h"
#include "tratsr.h"
#include "trim.h"
#include "tritsr.h"
#include "tscr.h"
#include "ttmc.h"


typedef struct {
    spinlock_t lock;

    void (*warn_handler)(void* ctx);
    void* warn_ctx;

    void (*critical_handler)(void* ctx);
    void* critical_ctx;

    bool warn_irq_enabled;
    bool critical_irq_enabled;
} tmu_state_t;


[[gnu::always_inline]] static inline tmu_state_t* get_state(
    driver_handle_t handle)
{
    return (tmu_state_t*)(*handle.state);
}

int32_t imx8mp_tmu_init(driver_handle_t handle)
{
    ASSERT(*handle.state == NULL, "TMU: Multiple initialization");

    *handle.state = kmalloc(sizeof(tmu_state_t));

    tmu_state_t* state = get_state(handle);

    *state = (tmu_state_t) {
        .lock                 = SPINLOCK_INIT,
        .warn_handler         = NULL,
        .warn_ctx             = NULL,
        .critical_handler     = NULL,
        .critical_ctx         = NULL,
        .warn_irq_enabled     = false,
        .critical_irq_enabled = false,
    };

    // Disable TMU before touching any register
    TmuTerValue ter = {0};
    TMU_TER_EN_set(&ter, false);
    TMU_TER_ADC_PD_set(&ter, false);
    TMU_TER_ALPF_set(&ter, TER_ALPF_VALUE_0_5);
    TMU_TER_write(handle.base, ter);

    // Both probes
    TmuTpsValue tps = {0};
    TMU_TPS_PROBE_SEL_set(&tps, TMU_TPS_PROBE_SEL_BOTH_PROBES);
    TMU_TPS_write(handle.base, tps);

    // Disable all thresholds
    TmuTmhtatrValue tmhtatr = {0};
    TMU_TMHTATR_EN0_set(&tmhtatr, false);
    TMU_TMHTATR_EN1_set(&tmhtatr, false);
    TMU_TMHTATR_write(handle.base, tmhtatr);

    TmuTmhtactrValue tmhtactr = {0};
    TMU_TMHTACTR_EN0_set(&tmhtactr, false);
    TMU_TMHTACTR_EN1_set(&tmhtactr, false);
    TMU_TMHTACTR_write(handle.base, tmhtactr);

    // Disable all IRQs
    TmuTierValue tier = {0};
    TMU_TIER_write(handle.base, tier);

    // Enable TMU
    TMU_TER_EN_set(&ter, true);
    TMU_TER_write(handle.base, ter);

    // Mandatory >=5us delay. TRM §5.4.3.1
    for (size_t i = 0; i < 20000; i++)
        asm volatile("nop");

    return 0;
}

int32_t imx8mp_tmu_exit(driver_handle_t handle)
{
    ASSERT(*handle.state != NULL, "TMU: Not initialized");

    tmu_state_t* state = get_state(handle);

    irq_spinlocked(&state->lock)
    {
        // Disable all IRQs and thresholds
        TmuTierValue tier = {0};
        TMU_TIER_write(handle.base, tier);

        TmuTmhtatrValue tmhtatr = TMU_TMHTATR_read(handle.base);
        TMU_TMHTATR_EN0_set(&tmhtatr, false);
        TMU_TMHTATR_EN1_set(&tmhtatr, false);
        TMU_TMHTATR_write(handle.base, tmhtatr);

        TmuTmhtactrValue tmhtactr = TMU_TMHTACTR_read(handle.base);
        TMU_TMHTACTR_EN0_set(&tmhtactr, false);
        TMU_TMHTACTR_EN1_set(&tmhtactr, false);
        TMU_TMHTACTR_write(handle.base, tmhtactr);

        TmuTerValue ter = TMU_TER_read(handle.base);
        TMU_TER_EN_set(&ter, false);
        TMU_TER_write(handle.base, ter);
    }

    kfree(*handle.state);
    *handle.state = NULL;

    return 0;
}

float imx8mp_tmu_read_current_celsius(driver_handle_t handle)
{
    TmuTratsrValue r = {0};

    bool   ready[2] = {false, false};
    int8_t temps[2] = {0, 0};

    size_t attempts = 0;
    while (1) {
        r = TMU_TRATSR_read(handle.base);

        if (!ready[0] && TMU_TRATSR_V0_get(r)) {
            ready[0] = true;
            temps[0] = TMU_TRATSR_TEMP0_get(r);
        }
        if (!ready[1] && TMU_TRATSR_V1_get(r)) {
            ready[1] = true;
            temps[1] = TMU_TRATSR_TEMP1_get(r);
        }

        if (ready[0] && ready[1])
            break;

        if (attempts++ >= 5)
            PANIC("TMU not responding");
    }

    int8_t max = (temps[0] > temps[1]) ? temps[0] : temps[1];
    return (float)max;
}

int32_t imx8mp_tmu_irq_enable(driver_handle_t handle)
{
    tmu_state_t* state = get_state(handle);

    irq_spinlocked(&state->lock)
    {
        TmuTierValue tier = TMU_TIER_read(handle.base);

        if (state->warn_handler) {
            TMU_TIER_ATTEIE0_set(&tier, true);
            TMU_TIER_ATTEIE1_set(&tier, true);
            state->warn_irq_enabled = true;
        }

        if (state->critical_handler) {
            TMU_TIER_ATCTEIE0_set(&tier, true);
            TMU_TIER_ATCTEIE1_set(&tier, true);
            state->critical_irq_enabled = true;
        }

        TMU_TIER_write(handle.base, tier);
    }

    return 0;
}

int32_t imx8mp_tmu_irq_disable(driver_handle_t handle)
{
    tmu_state_t* state = get_state(handle);

    irq_spinlocked(&state->lock)
    {
        TmuTierValue tier = TMU_TIER_read(handle.base);
        TMU_TIER_ATTEIE0_set(&tier, false);
        TMU_TIER_ATTEIE1_set(&tier, false);
        TMU_TIER_ATCTEIE0_set(&tier, false);
        TMU_TIER_ATCTEIE1_set(&tier, false);
        TMU_TIER_write(handle.base, tier);

        state->warn_irq_enabled     = false;
        state->critical_irq_enabled = false;
    }

    return 0;
}

int32_t imx8mp_tmu_irq_notify_warn_over(
    driver_handle_t handle,
    float           threshold,
    void (*handler)(void* ctx),
    void* ctx)
{
    tmu_state_t* state  = get_state(handle);
    int8_t       temp_c = (int8_t)threshold;

    irq_spinlocked(&state->lock)
    {
        state->warn_handler = handler;
        state->warn_ctx     = ctx;

        // Disable warn IRQ while reconfiguring
        TmuTierValue tier = TMU_TIER_read(handle.base);
        TMU_TIER_ATTEIE0_set(&tier, false);
        TMU_TIER_ATTEIE1_set(&tier, false);
        TMU_TIER_write(handle.base, tier);

        // Reconfigure threshold. TRM §5.4.3.1
        TmuTerValue ter = TMU_TER_read(handle.base);
        TMU_TER_EN_set(&ter, false);
        TMU_TER_write(handle.base, ter);

        TmuTmhtatrValue tmhtatr = TMU_TMHTATR_read(handle.base);
        TMU_TMHTATR_EN0_set(&tmhtatr, false);
        TMU_TMHTATR_EN1_set(&tmhtatr, false);
        TMU_TMHTATR_write(handle.base, tmhtatr);

        TMU_TMHTATR_TEMP0_set(&tmhtatr, temp_c);
        TMU_TMHTATR_TEMP1_set(&tmhtatr, temp_c);
        TMU_TMHTATR_write(handle.base, tmhtatr);

        TMU_TER_EN_set(&ter, true);
        TMU_TER_write(handle.base, ter);

        for (size_t i = 0; i < 20000; i++)
            asm volatile("nop");

        TMU_TMHTATR_EN0_set(&tmhtatr, true);
        TMU_TMHTATR_EN1_set(&tmhtatr, true);
        TMU_TMHTATR_write(handle.base, tmhtatr);

        // Re-arm IRQ only if globally enabled
        if (state->warn_irq_enabled) {
            TMU_TIER_ATTEIE0_set(&tier, true);
            TMU_TIER_ATTEIE1_set(&tier, true);
            TMU_TIER_write(handle.base, tier);
        }
    }

    return 0;
}

int32_t imx8mp_tmu_irq_notify_critical_over(
    driver_handle_t handle,
    float           threshold,
    void (*handler)(void* ctx),
    void* ctx)
{
    tmu_state_t* state  = get_state(handle);
    int8_t       temp_c = (int8_t)threshold;

    irq_spinlocked(&state->lock)
    {
        state->critical_handler = handler;
        state->critical_ctx     = ctx;

        TmuTierValue tier = TMU_TIER_read(handle.base);
        TMU_TIER_ATCTEIE0_set(&tier, false);
        TMU_TIER_ATCTEIE1_set(&tier, false);
        TMU_TIER_write(handle.base, tier);

        TmuTerValue ter = TMU_TER_read(handle.base);
        TMU_TER_EN_set(&ter, false);
        TMU_TER_write(handle.base, ter);

        TmuTmhtactrValue tmhtactr = TMU_TMHTACTR_read(handle.base);
        TMU_TMHTACTR_EN0_set(&tmhtactr, false);
        TMU_TMHTACTR_EN1_set(&tmhtactr, false);
        TMU_TMHTACTR_write(handle.base, tmhtactr);

        TMU_TMHTACTR_TEMP0_set(&tmhtactr, temp_c);
        TMU_TMHTACTR_TEMP1_set(&tmhtactr, temp_c);
        TMU_TMHTACTR_write(handle.base, tmhtactr);

        TMU_TER_EN_set(&ter, true);
        TMU_TER_write(handle.base, ter);

        for (size_t i = 0; i < 20000; i++)
            asm volatile("nop");

        TMU_TMHTACTR_EN0_set(&tmhtactr, true);
        TMU_TMHTACTR_EN1_set(&tmhtactr, true);
        TMU_TMHTACTR_write(handle.base, tmhtactr);

        if (state->critical_irq_enabled) {
            TMU_TIER_ATCTEIE0_set(&tier, true);
            TMU_TIER_ATCTEIE1_set(&tier, true);
            TMU_TIER_write(handle.base, tier);
        }
    }

    return 0;
}

void imx8mp_tmu_irq_handle(driver_handle_t handle)
{
    tmu_state_t* state = get_state(handle);

    TmuTidrValue tidr = TMU_TIDR_read(handle.base);

    bool warn0 = TMU_TIDR_ATTE0_get(tidr);
    bool warn1 = TMU_TIDR_ATTE1_get(tidr);
    bool crit0 = TMU_TIDR_ATCTE0_get(tidr);
    bool crit1 = TMU_TIDR_ATCTE1_get(tidr);

    // Ack everything seen in one write (TIDR is w1c)
    TmuTidrValue ack = {0};
    TMU_TIDR_ATTE0_set(&ack, warn0);
    TMU_TIDR_ATTE1_set(&ack, warn1);
    TMU_TIDR_ATCTE0_set(&ack, crit0);
    TMU_TIDR_ATCTE1_set(&ack, crit1);
    TMU_TIDR_write(handle.base, ack);

    if (warn0 || warn1) {
        // Disable warn IRQ until re-enabled, matching original behaviour
        TmuTierValue tier = TMU_TIER_read(handle.base);
        TMU_TIER_ATTEIE0_set(&tier, false);
        TMU_TIER_ATTEIE1_set(&tier, false);
        TMU_TIER_write(handle.base, tier);
        state->warn_irq_enabled = false;

        if (state->warn_handler)
            state->warn_handler(state->warn_ctx);
    }

    if (crit0 || crit1) {
        TmuTierValue tier = TMU_TIER_read(handle.base);
        TMU_TIER_ATCTEIE0_set(&tier, false);
        TMU_TIER_ATCTEIE1_set(&tier, false);
        TMU_TIER_write(handle.base, tier);
        state->critical_irq_enabled = false;

        if (state->critical_handler)
            state->critical_handler(state->critical_ctx);
    }
}

static const thermal_sensor_ops_t OPS = {
    .init                      = imx8mp_tmu_init,
    .exit                      = imx8mp_tmu_exit,
    .precission                = 1.0f,
    .min_celsius               = -40.0f,
    .max_celsius               = 105.0f,
    .read_current_celsius      = imx8mp_tmu_read_current_celsius,
    .irq_enable                = imx8mp_tmu_irq_enable,
    .irq_disable               = imx8mp_tmu_irq_disable,
    .irq_notify_warn_over      = imx8mp_tmu_irq_notify_warn_over,
    .irq_notify_warn_under     = NULL,
    .irq_notify_critical_over  = imx8mp_tmu_irq_notify_critical_over,
    .irq_notify_critical_under = NULL,
    .irq_handle                = imx8mp_tmu_irq_handle,
};

const thermal_sensor_ops_t* const IMX8MP_THERMAL_MONITORING_UNIT_OPS = &OPS;