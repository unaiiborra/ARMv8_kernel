#include <drivers/tmu/tmu.h>
#include <drivers/tmu/tmu_raw.h>
#include <kernel/panic.h>
#include <lib/lock.h>
#include <lib/stdmacros.h>

const int8_t TMU_MAX_SENSOR_TEMP_C = 125;
const int8_t TMU_MIN_SENSOR_TEMP_C = -40;

const int8_t TMU_MAX_RECCOMENDED_TEMP_C = 85;

typedef enum {
    TMU_NOT_INITIALIZED = 0,
    TMU_STAGE0_INITIALIZED,
    TMU_STAGE1_INITIALIZED,
    TMU_FULLY_INITIALIZED = TMU_STAGE1_INITIALIZED,
} tmu_init_stage;

// Represents the bits in the local copy of enabled irqs in the state (bitfield8
// irq_status)
typedef enum {
    TMU_IRQ_START = 0,
    // Instantaneous warn
    TMU_IRQ_ITTE0 = 0,
    TMU_IRQ_ITTE1,
    // Avg warn
    TMU_IRQ_ATTE0,
    TMU_IRQ_ATTE1,
    // Avg critical
    TMU_IRQ_ATCTE0,
    TMU_IRQ_ATCTE1,

    TMU_IRQ_COUNT,
} tmu_irq;

extern void _TMU_critical_hang(const driver_handle*, uint64_t);

#ifdef TEST
static inline void TMU_panic_if_uninit_(const driver_handle* h)
{
    tmu_state* state = (tmu_state*)h->state;

    if (state->init_stage != TMU_FULLY_INITIALIZED)
        PANIC("MU not fully initialized and attempted to be used");
};
#endif

// Helper that checks if the TMU has been initialized and returns the cast of
// the state as a helper
static inline tmu_state* TMU_get_state_(const driver_handle* h)
{
    tmu_state* state = (tmu_state*)h->state;

#ifdef TEST
    TMU_panic_if_uninit_(h);
#endif

    return state;
}

static inline void TMU_disable_(const driver_handle* h)
{
    TmuTerValue r = TMU_TER_read(h->base);

    TMU_TER_EN_set(&r, false);
    TMU_TER_ADC_PD_set(&r, false);
    TMU_TER_write(h->base, r);
}

static inline void TMU_enable_(const driver_handle* h)
{
    TmuTerValue r = TMU_TER_read(h->base);

    TMU_TER_EN_set(&r, true);
    TMU_TER_ADC_PD_set(&r, false);
    TMU_TER_write(h->base, r);
}

static inline int8_t TMU_validate_max_temp_(int8_t temp_c)
{
    temp_c =
        (temp_c <= TMU_MAX_SENSOR_TEMP_C) ? temp_c : TMU_MAX_RECCOMENDED_TEMP_C;
    temp_c =
        (temp_c >= TMU_MIN_SENSOR_TEMP_C) ? temp_c : TMU_MAX_RECCOMENDED_TEMP_C;
    return temp_c;
}

static inline bool TMU_get_irq_status_(const driver_handle* h, tmu_irq irq)
{
    tmu_state* state = (tmu_state*)h->state;

    return bitfield_get((state->irq_status), irq);
}

static inline void
TMU_set_irq_status_(const driver_handle* h, tmu_irq irq, bool enable)
{
    if (enable == TMU_get_irq_status_(h, irq))
        return;

    tmu_state* state = (tmu_state*)h->state;

    TmuTierValue tier = TMU_TIER_read(h->base);
    switch (irq) {
        case TMU_IRQ_ITTE0:
            TMU_TIER_ITTEIE0_set(&tier, enable);
            enable ? bitfield_set_high((state->irq_status), TMU_IRQ_ITTE0)
                   : bitfield_clear((state->irq_status), TMU_IRQ_ITTE0);
            break;
        case TMU_IRQ_ITTE1:
            TMU_TIER_ITTEIE1_set(&tier, enable);
            enable ? bitfield_set_high((state->irq_status), TMU_IRQ_ITTE1)
                   : bitfield_clear((state->irq_status), TMU_IRQ_ITTE1);
            break;
        case TMU_IRQ_ATTE0:
            TMU_TIER_ATTEIE0_set(&tier, enable);
            enable ? bitfield_set_high((state->irq_status), TMU_IRQ_ATTE0)
                   : bitfield_clear((state->irq_status), TMU_IRQ_ATTE0);
            break;
        case TMU_IRQ_ATTE1:
            TMU_TIER_ATTEIE1_set(&tier, enable);
            enable ? bitfield_set_high((state->irq_status), TMU_IRQ_ATTE1)
                   : bitfield_clear((state->irq_status), TMU_IRQ_ATTE1);
            break;
        case TMU_IRQ_ATCTE0:
            TMU_TIER_ATCTEIE0_set(&tier, enable);
            enable ? bitfield_set_high((state->irq_status), TMU_IRQ_ATCTE0)
                   : bitfield_clear((state->irq_status), TMU_IRQ_ATCTE0);
            break;
        case TMU_IRQ_ATCTE1:
            TMU_TIER_ATCTEIE1_set(&tier, enable);
            enable ? bitfield_set_high((state->irq_status), TMU_IRQ_ATCTE1)
                   : bitfield_clear((state->irq_status), TMU_IRQ_ATCTE1);
            break;
        default:
            PANIC("");
    }

    TMU_TIER_write(h->base, tier);
}

// Disables/enables both irq0 and 1 of the status, to avoid an irq arriving
// while disabling one with TMU_set_irq_status and the other one not being
// disabled.
static inline void
TMU_ATTEX_irq_status_set_(const driver_handle* h, bool atte0, bool atte1)
{
    tmu_state* state = TMU_get_state_(h);

    if (atte0 == TMU_get_irq_status_(h, TMU_IRQ_ATTE0) &&
        atte1 == TMU_get_irq_status_(h, TMU_IRQ_ATTE1))
        return;

    TmuTierValue tier = TMU_TIER_read(h->base);
    TMU_TIER_ATTEIE0_set(&tier, atte0);
    TMU_TIER_ATTEIE1_set(&tier, atte1);
    TMU_TIER_write(h->base, tier);

    atte0 ? bitfield_set_high((state->irq_status), TMU_IRQ_ATTE0)
          : bitfield_clear((state->irq_status), TMU_IRQ_ATTE0);
    atte1 ? bitfield_set_high((state->irq_status), TMU_IRQ_ATTE1)
          : bitfield_clear((state->irq_status), TMU_IRQ_ATTE1);
}

static inline bitfield8 TMU_get_irq_sources_(const driver_handle* h)
{
    TmuTidrValue tidr = TMU_TIDR_read(h->base);

    bitfield8 sources = 0;

#define SET_SRC(bit, status) \
    sources |= ((bitfield8)(((status) & TMU_get_irq_status_(h, bit)) << (bit)))

    SET_SRC(TMU_IRQ_ITTE0, TMU_TIDR_ITTE0_get(tidr));
    SET_SRC(TMU_IRQ_ITTE1, TMU_TIDR_ITTE1_get(tidr));
    SET_SRC(TMU_IRQ_ATTE0, TMU_TIDR_ATTE0_get(tidr));
    SET_SRC(TMU_IRQ_ATTE1, TMU_TIDR_ATTE1_get(tidr));
    SET_SRC(TMU_IRQ_ATCTE0, TMU_TIDR_ATCTE0_get(tidr));
    SET_SRC(TMU_IRQ_ATCTE1, TMU_TIDR_ATCTE1_get(tidr));

#undef SET_SRC
    return sources;
}

static inline void TMU_ack_irq_(const driver_handle* h, tmu_irq irq)
{
    TmuTidrValue r = {0};

    switch (irq) {
        case TMU_IRQ_ITTE0:
            TMU_TIDR_ITTE0_set(&r, true);
            break;
        case TMU_IRQ_ITTE1:
            TMU_TIDR_ITTE1_set(&r, true);
            break;
        case TMU_IRQ_ATTE0:
            TMU_TIDR_ATTE0_set(&r, true);
            break;
        case TMU_IRQ_ATTE1:
            TMU_TIDR_ATTE1_set(&r, true);
            break;
        case TMU_IRQ_ATCTE0:
            TMU_TIDR_ATCTE0_set(&r, true);
            break;
        case TMU_IRQ_ATCTE1:
            TMU_TIDR_ATCTE1_set(&r, true);
            break;
        default:
            PANIC("");
    }

    TMU_TIDR_write(h->base, r);
}

void TMU_init_stage0(const driver_handle* h, tmu_cfg cfg)
{
    tmu_state* state = (tmu_state*)h->state;

    if (state->init_stage != TMU_NOT_INITIALIZED)
        PANIC("Driver initialized multiple times");

    state->cfg          = cfg;
    state->irq_status   = 0;
    state->warn_pending = false;
    state->init_stage   = TMU_NOT_INITIALIZED;

    TMU_disable_(h);

    TmuTerValue ter = {0};
    TMU_TER_EN_set(&ter, false);
    TMU_TER_ADC_PD_set(&ter, false);
    TMU_TER_ALPF_set(&ter, TER_ALPF_VALUE_0_5); // 0.5 low pass filter
    TMU_TER_write(h->base, ter);

    TmuTpsValue tps = {0};
    TMU_TPS_PROBE_SEL_set(&tps, TMU_TPS_PROBE_SEL_BOTH_PROBES);
    TMU_TPS_write(h->base, tps);

    TMU_enable_(h);

    state->init_stage = TMU_STAGE0_INITIALIZED;
}

void TMU_init_stage1(const driver_handle* h)
{
    // 5.4.3.1 680
    // State should have been declared before in stage1 init

    tmu_state* state = (tmu_state*)h->state;

    if (state->init_stage != TMU_STAGE0_INITIALIZED)
        PANIC(
            "Driver initialized multiple times or not initialized from stage "
            "0");

    TMU_disable_(h);

    // Disable average threashold
    TmuTmhtatrValue tmhtatr = TMU_TMHTATR_read(h->base);
    TMU_TMHTATR_EN0_set(&tmhtatr, false);
    TMU_TMHTATR_EN1_set(&tmhtatr, false);
    TMU_TMHTATR_write(h->base, tmhtatr);

    // Disable critical threashold
    TmuTmhtactrValue tmhtactr = TMU_TMHTACTR_read(h->base);
    TMU_TMHTACTR_EN0_set(&tmhtactr, false);
    TMU_TMHTACTR_EN1_set(&tmhtactr, false);
    TMU_TMHTACTR_write(h->base, tmhtactr);

    // Set new average treashold
    int8_t warn_max = TMU_validate_max_temp_(state->cfg.warn_max);
    TMU_TMHTATR_TEMP0_set(&tmhtatr, warn_max);
    TMU_TMHTATR_TEMP1_set(&tmhtatr, warn_max);
    TMU_TMHTATR_write(h->base, tmhtatr);

    // Set new critical treashold
    int8_t critical_max = TMU_validate_max_temp_(state->cfg.critical_max);
    TMU_TMHTACTR_TEMP0_set(&tmhtactr, critical_max);
    TMU_TMHTACTR_TEMP1_set(&tmhtactr, critical_max);
    TMU_TMHTACTR_write(h->base, tmhtactr);

    TMU_enable_(h); // Enable the tmu

    // A delay of at least 5us is needed here to reset the TMU internal states
    // of the last run. Otherwise, the old values might still be used, leading
    // to unexpected results.
    for (size_t i = 0; i < 20000; i++)
        asm volatile("nop");

    // Enable warn threashold
    TMU_TMHTATR_EN0_set(&tmhtatr, true);
    TMU_TMHTATR_EN1_set(&tmhtatr, true);
    TMU_TMHTATR_write(h->base, tmhtatr);

    // Enable critical threashold
    TMU_TMHTACTR_EN0_set(&tmhtactr, true);
    TMU_TMHTACTR_EN1_set(&tmhtactr, true);
    TMU_TMHTACTR_write(h->base, tmhtactr);

    // Enable irqs
    TMU_set_irq_status_(h, TMU_IRQ_ATTE0, true);
    TMU_set_irq_status_(h, TMU_IRQ_ATTE1, true);
    TMU_set_irq_status_(h, TMU_IRQ_ATCTE0, true);
    TMU_set_irq_status_(h, TMU_IRQ_ATCTE1, true);

    state->init_stage = TMU_STAGE1_INITIALIZED;
}

typedef void (
    *tmu_irq_handler)(const driver_handle* h, uint64_t probe); // probe 0 or 1

// Irq handlers
static void TMU_unhandled_irq_(const driver_handle*, uint64_t probe)
{
    char* panic_msg = "Unhandled irq from probe0";

    if (probe % 2 != 0)
        panic_msg = "Unhandled irq from probe1";

    PANIC(panic_msg);
}

// warning irq handler
static void TMU_ATTEX_irq_handler_(const driver_handle* h, uint64_t probe)
{
#ifdef TEST
    TMU_panic_if_uninit_(h);
#endif

    tmu_state* state = TMU_get_state_(h);

    TMU_ack_irq_(h, (probe == 0) ? TMU_IRQ_ATTE0 : TMU_IRQ_ATTE1);

    spinlocked(&state->state_lock)
    { // It should never be locked by the same
        // core as all the other locks use irqsave
        TMU_ATTEX_irq_status_set_(h, false, false);
        state->warn_pending = true;
    }
}

static const tmu_irq_handler TMU_IRQ_HANDLERS_[TMU_IRQ_COUNT] = {
    [TMU_IRQ_ITTE0] = TMU_unhandled_irq_,
    [TMU_IRQ_ITTE1] = TMU_unhandled_irq_,
    [TMU_IRQ_ATTE0] = TMU_ATTEX_irq_handler_,
    [TMU_IRQ_ATTE1] = TMU_ATTEX_irq_handler_,
    // If executed with TF-A it will never reach this point, the firmware will
    // automatically take control
    [TMU_IRQ_ATCTE0] = _TMU_critical_hang,
    [TMU_IRQ_ATCTE1] = _TMU_critical_hang,
};

void TMU_handle_irq(const driver_handle* h)
{
#ifdef TEST
    TMU_panic_if_uninit_(h);
#endif

    bitfield8 irq_sources = TMU_get_irq_sources_(h);

    for (tmu_irq irq = TMU_IRQ_START; irq < TMU_IRQ_COUNT; irq++) {
        if (bitfield_get(irq_sources, irq))
            // The enum is defined so probe 0 is even and probe 1 is odd
            TMU_IRQ_HANDLERS_[irq](h, irq % 2);
    }
}

/*
 *  --- Driver interface fns ---
 */
void TMU_set_warn_temp(const driver_handle* h, int8_t temp_c)
{
#ifdef TEST
    TMU_panic_if_uninit_(h);
#endif
    tmu_state* state = TMU_get_state_(h);

    irq_spinlocked(&state->state_lock)
    {
        bool atte0_irq = TMU_get_irq_status_(h, TMU_IRQ_ATTE0);
        bool atte1_irq = TMU_get_irq_status_(h, TMU_IRQ_ATTE1);

        // They already have early returns if it is already disabled
        TMU_ATTEX_irq_status_set_(h, false, false);

        if (temp_c != TMU_validate_max_temp_(temp_c))
            PANIC("Invalid temperature provided, out of sensor range");

        TMU_disable_(h);

        // Disable average threashold
        TmuTmhtatrValue tmhtatr = TMU_TMHTATR_read(h->base);
        TMU_TMHTATR_EN0_set(&tmhtatr, false);
        TMU_TMHTATR_EN1_set(&tmhtatr, false);
        TMU_TMHTATR_write(h->base, tmhtatr);

        // Set new average treashold
        TMU_TMHTATR_TEMP0_set(&tmhtatr, temp_c);
        TMU_TMHTATR_TEMP1_set(&tmhtatr, temp_c);
        TMU_TMHTATR_write(h->base, tmhtatr);

        TMU_enable_(h); // Enable the tmu

        // A delay of at least 5us is needed here to reset the TMU internal
        // states of the last run. Otherwise, the old values might still be
        // used, leading to unexpected results.
        for (size_t i = 0; i < 20000; i++)
            asm volatile("nop");

        // Enable warn threashold
        TMU_TMHTATR_EN0_set(&tmhtatr, true);
        TMU_TMHTATR_EN1_set(&tmhtatr, true);
        TMU_TMHTATR_write(h->base, tmhtatr);

        state->cfg.warn_max = temp_c;

        TMU_ATTEX_irq_status_set_(h, atte0_irq, atte1_irq);
    }
}

bool TMU_get_warnings_enabled(const driver_handle* h)
{
    tmu_state* state = TMU_get_state_(h);

    irq_spinlocked(&state->state_lock)
    {
        return TMU_get_irq_status_(h, TMU_IRQ_ATTE0) ||
               TMU_get_irq_status_(h, TMU_IRQ_ATTE1);
    }

    PANIC("");
}

void TMU_enable_warnings(const driver_handle* h)
{
    tmu_state* state = TMU_get_state_(h);

    irq_spinlocked(&state->state_lock)
    {
        TMU_ATTEX_irq_status_set_(h, true, true);
    }
}

void TMU_disable_warnings(const driver_handle* h)
{
    tmu_state* state = TMU_get_state_(h);

    irq_spinlocked(&state->state_lock)
    {
        TMU_ATTEX_irq_status_set_(h, false, false);
    }
}

// Tells if the warning temperature threashold was reached, calling this
// function disables the pending state. When a warning arrives, the warning irq
// is automatically disabled.
bool TMU_warn_pending(const driver_handle* h)
{
    tmu_state* state = TMU_get_state_(h);

    bool pending;

    irq_spinlocked(&state->state_lock)
    {
        pending             = state->warn_pending;
        state->warn_pending = false; // Ack the warning, the kernel must reinit

        // the irqs if it wants new warnings
    }

    return pending;
}

// Critical irq allways active, TF-A will shut down the cpu automatically
void TMU_set_critical_temp(const driver_handle* h, int8_t temp_c)
{
#ifdef TEST
    TMU_panic_if_uninit_(h);
#endif

    tmu_state* state = TMU_get_state_(h);

    irq_spinlocked(&state->state_lock)
    {
        TMU_set_irq_status_(h, TMU_IRQ_ATCTE0, false);
        TMU_set_irq_status_(h, TMU_IRQ_ATCTE1, false);

        if (temp_c != TMU_validate_max_temp_(temp_c))
            PANIC("Invalid temperature provided, out of sensor range");

        TMU_disable_(h);

        // Disable average threashold
        TmuTmhtactrValue tmhtactr = TMU_TMHTACTR_read(h->base);
        TMU_TMHTACTR_EN0_set(&tmhtactr, false);
        TMU_TMHTACTR_EN1_set(&tmhtactr, false);
        TMU_TMHTACTR_write(h->base, tmhtactr);

        // Set new average treashold
        TMU_TMHTACTR_TEMP0_set(&tmhtactr, temp_c);
        TMU_TMHTACTR_TEMP1_set(&tmhtactr, temp_c);
        TMU_TMHTACTR_write(h->base, tmhtactr);

        TMU_enable_(h); // Enable the tmu

        // A delay of at least 5us is needed here to reset the TMU internal
        // states of the last run. Otherwise, the old values might still be
        // used, leading to unexpected results.
        for (size_t i = 0; i < 20000; i++)
            asm volatile("nop");

        // Enable warn threashold
        TMU_TMHTACTR_EN0_set(&tmhtactr, true);
        TMU_TMHTACTR_EN1_set(&tmhtactr, true);
        TMU_TMHTACTR_write(h->base, tmhtactr);

        state->cfg.critical_max = temp_c;

        TMU_set_irq_status_(h, TMU_IRQ_ATCTE0, true);
        TMU_set_irq_status_(h, TMU_IRQ_ATCTE1, true);
    }
}

int8_t TMU_get_temp(const driver_handle* h)
{
#ifdef TEST
    TMU_get_state_(h); // panics if not init

    TmuTpsValue ter = TMU_TPS_read(h->base);
    if (TMU_TPS_PROBE_SEL_get(ter) != TMU_TPS_PROBE_SEL_BOTH_PROBES)
        PANIC("TMU not initialized correctly");
#endif

    TmuTratsrValue r = {0};

    bool   ready[2] = {false, false};
    int8_t temps[2] = {0, 0};

    size_t attempts = 0;
    while (1) {
        r = TMU_TRATSR_read(h->base);

        for (size_t i = 0; i < 2; i++) {
            if (!ready[i]) {
                switch (i) {
                    case 0:
                        ready[i] = TMU_TRATSR_V0_get(r);
                        break;
                    case 1:
                        ready[i] = TMU_TRATSR_V1_get(r);
                }

                if (ready[i]) {
                    switch (i) {
                        case 0:
                            temps[i] = TMU_TRATSR_TEMP0_get(r);
                            break;
                        case 1:
                            temps[i] = TMU_TRATSR_TEMP1_get(r);
                    }
                }
            }
        }

        if (ready[0] && ready[1])
            break;

        if (attempts++ >= 5)
            PANIC("TMU not responding");
    }

    int8_t max = (temps[0] > temps[1]) ? temps[0] : temps[1];
    return max;
}
