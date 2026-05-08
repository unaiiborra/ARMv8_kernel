#include <arm/exceptions/exception_class/data_abort.h>
#include <arm/exceptions/sync.h>
#include <kernel/io/stdio.h>
#include <kernel/mm/page_fault.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/sysregs/sysregs.h"
#include "kernel/io/stdio.h"
#include "kernel/mm.h"
#include "kernel/mm/uregion.h"
#include "kernel/panic.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "lib/align.h"
#include "lib/branch.h"
#include "lib/mem.h"
#include "lib/stdattribute.h"

#define case_print(v)                        \
    dbg_printf(                              \
        DEBUG_TRACE,                         \
        "[page_fault_handler] " #v " at %p", \
        get_fault_address());


typedef enum dfsc {
    // https://developer.arm.com/documentation/ddi0601/2021-03/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-?lang=en#fieldset_0-25_25
    /* Address size faults */
    DFSC_ADDR_SIZE_L0 = 0b000000, /* Address size fault, level 0 / TTBRn_EL1 */
    DFSC_ADDR_SIZE_L1 = 0b000001, /* Address size fault, level 1              */
    DFSC_ADDR_SIZE_L2 = 0b000010, /* Address size fault, level 2              */
    DFSC_ADDR_SIZE_L3 = 0b000011, /* Address size fault, level 3              */
    DFSC_ADDR_SIZE_Lm1 = 0b101001, /* Address size fault, level -1 [FEAT_LPA2] */

    /* Translation faults */
    DFSC_TRANSLATION_Lm1 = 0b101011, /* Translation fault, level -1 [FEAT_LPA2] */
    DFSC_TRANSLATION_L0 = 0b000100, /* Translation fault, level 0 */
    DFSC_TRANSLATION_L1 = 0b000101, /* Translation fault, level 1 */
    DFSC_TRANSLATION_L2 = 0b000110, /* Translation fault, level 2 */
    DFSC_TRANSLATION_L3 = 0b000111, /* Translation fault, level 3 */

    /* Access flag faults */
    DFSC_ACCESS_FLAG_L0 = 0b001000, /* Access flag fault, level 0 [FEAT_LPA2] */
    DFSC_ACCESS_FLAG_L1 = 0b001001, /* Access flag fault, level 1             */
    DFSC_ACCESS_FLAG_L2 = 0b001010, /* Access flag fault, level 2             */
    DFSC_ACCESS_FLAG_L3 = 0b001011, /* Access flag fault, level 3             */

    /* Permission faults */
    DFSC_PERMISSION_L0 = 0b001100, /* Permission fault, level 0 [FEAT_LPA2] */
    DFSC_PERMISSION_L1 = 0b001101, /* Permission fault, level 1             */
    DFSC_PERMISSION_L2 = 0b001110, /* Permission fault, level 2             */
    DFSC_PERMISSION_L3 = 0b001111, /* Permission fault, level 3             */

    /* Synchronous external aborts — not on TTW */
    DFSC_SYNC_EXT_ABORT = 0b010000, /* Synchronous external abort, not on TTW */
    DFSC_SYNC_TAG_CHECK = 0b010001, /* Synchronous Tag Check fault [FEAT_MTE2] */

    /* Synchronous external aborts — on TTW */
    DFSC_SYNC_EXT_ABORT_TTW_Lm1 = 0b010011, /* Sync ext abort on TTW, level -1
                                               [FEAT_LPA2]  */
    DFSC_SYNC_EXT_ABORT_TTW_L0 = 0b010100, /* Sync ext abort on TTW, level 0  */
    DFSC_SYNC_EXT_ABORT_TTW_L1 = 0b010101, /* Sync ext abort on TTW, level 1  */
    DFSC_SYNC_EXT_ABORT_TTW_L2 = 0b010110, /* Sync ext abort on TTW, level 2  */
    DFSC_SYNC_EXT_ABORT_TTW_L3 = 0b010111, /* Sync ext abort on TTW, level 3  */

    /* Synchronous parity / ECC errors [!FEAT_RAS] — not on TTW */
    DFSC_SYNC_PARITY = 0b011000, /* Sync parity/ECC error, not on TTW
                                    [!FEAT_RAS]   */

    /* Synchronous parity / ECC errors [!FEAT_RAS] — on TTW */
    DFSC_SYNC_PARITY_TTW_Lm1 = 0b011011, /* Sync parity/ECC on TTW, level -1
                                            [FEAT_LPA2, !FEAT_RAS] */
    DFSC_SYNC_PARITY_TTW_L0 = 0b011100,  /* Sync parity/ECC on TTW, level 0
                                            [!FEAT_RAS]            */
    DFSC_SYNC_PARITY_TTW_L1 = 0b011101,  /* Sync parity/ECC on TTW, level 1
                                            [!FEAT_RAS]            */
    DFSC_SYNC_PARITY_TTW_L2 = 0b011110,  /* Sync parity/ECC on TTW, level 2
                                            [!FEAT_RAS]            */
    DFSC_SYNC_PARITY_TTW_L3 = 0b011111,  /* Sync parity/ECC on TTW, level 3
                                            [!FEAT_RAS]            */

    /* Alignment */
    DFSC_ALIGNMENT = 0b100001, /* Alignment fault */

    /* Granule Protection Faults [FEAT_RME] */
    DFSC_GPF_TTW_Lm1 = 0b100011, /* GPF on TTW, level -1 [FEAT_RME, FEAT_LPA2] */
    DFSC_GPF_TTW_L0 = 0b100100, /* GPF on TTW, level 0  [FEAT_RME]            */
    DFSC_GPF_TTW_L1 = 0b100101, /* GPF on TTW, level 1  [FEAT_RME]            */
    DFSC_GPF_TTW_L2 = 0b100110, /* GPF on TTW, level 2  [FEAT_RME]            */
    DFSC_GPF_TTW_L3 = 0b100111, /* GPF on TTW, level 3  [FEAT_RME]            */
    DFSC_GPF        = 0b101000, /* GPF, not on TTW      [FEAT_RME]            */

    /* Miscellaneous */
    DFSC_TLB_CONFLICT          = 0b110000, /* TLB conflict abort */
    DFSC_UNSUPPORTED_ATOMIC_HW = 0b110001, /* Unsupported atomic hardware update
                                              [FEAT_HAFDBS]      */
    DFSC_IMPDEF_LOCKDOWN    = 0b110100, /* IMPLEMENTATION DEFINED — Lockdown */
    DFSC_IMPDEF_EXCL_ATOMIC = 0b110101, /* IMPLEMENTATION DEFINED — Unsupported
                                           Exclusive/Atomic */
} dfsc_t;


[[gnu::always_inline]] static inline uintptr_t get_fault_address()
{
    return sysreg_read(far_el1);
}

[[gnu::always_inline]] static inline size_t get_dabt_word_size(
    const data_abort_iss* iss)
{
    if (likely(iss->ISV == 1))
        switch (iss->SAS) {
            case 0b00: // Byte
                return 1;
            case 0b01:
                return 2;
            case 0b10:
                return 4;
            case 0b11:
                return 8;
        }

    return 0;
}


static void translation_fault(
    [[maybe_unused]] int32_t level,
    data_abort_iss*          iss)
{
    uregion_t* uregion;
    task_t*    task           = get_current_thread()->owner;
    uintptr_t  fault_address  = get_fault_address();
    size_t     dabt_word_size = get_dabt_word_size(iss);


    bool reserved_uregion = uregion_is_reserved(
        task,
        fault_address,
        dabt_word_size == 0 ? 1 : dabt_word_size,
        &uregion);

    if (likely(reserved_uregion)) {
        void* kva = uregion_commit(
            task,
            align_down(fault_address, PAGE_ALIGN),
            1);

        memzero64(kva, PAGE_SIZE);

        return;
    }

    // unreserved region access TODO: SIGNAL
    terminate_task(2);
}




void page_fault_handler()
{
    uint64_t esr = sysreg_read(esr_el1);

    [[maybe_unused]] esr_ec exception_class = ESR_EC(esr);
    uint64_t                il              = ESR_IL(esr); // instruction lenght
    uint64_t                iss             = ESR_ISS(esr);
    // uint64_t iss2 = ESR_ISS2(esr_el1);

    DEBUG_ASSERT(exception_class == ESR_EC_DABT_LOWER_EL);
    DEBUG_ASSERT(il == 1, "16-bit instructions not supported");

    data_abort_iss dabt_iss = data_abort_iss_new(iss);


    switch ((dfsc_t)dabt_iss.DFSC) {
        /* ── Address size faults ─────────────────────────────────────────── */
        case DFSC_ADDR_SIZE_L0: {
            case_print(DFSC_ADDR_SIZE_L0);
            terminate_task(1);
        } break;
        case DFSC_ADDR_SIZE_L1: {
            case_print(DFSC_ADDR_SIZE_L1);
            terminate_task(1);
        } break;
        case DFSC_ADDR_SIZE_L2: {
            case_print(DFSC_ADDR_SIZE_L2);
            terminate_task(1);
        } break;
        case DFSC_ADDR_SIZE_L3: {
            case_print(DFSC_ADDR_SIZE_L3);
            terminate_task(1);
        } break;
        case DFSC_ADDR_SIZE_Lm1: {
            case_print(DFSC_ADDR_SIZE_Lm1);
            terminate_task(1);
        } break;

        /* ── Translation faults ──────────────────────────────────────────── */
        case DFSC_TRANSLATION_Lm1: {
            case_print(DFSC_TRANSLATION_Lm1);
            translation_fault(-1, &dabt_iss);
        } break;
        case DFSC_TRANSLATION_L0: {
            case_print(DFSC_TRANSLATION_L0);
            translation_fault(0, &dabt_iss);
        } break;
        case DFSC_TRANSLATION_L1: {
            case_print(DFSC_TRANSLATION_L1);
            translation_fault(1, &dabt_iss);
        } break;
        case DFSC_TRANSLATION_L2: {
            case_print(DFSC_TRANSLATION_L2);
            translation_fault(2, &dabt_iss);
        } break;
        case DFSC_TRANSLATION_L3: {
            case_print(DFSC_TRANSLATION_L3);
            translation_fault(3, &dabt_iss);
        } break;

        /* ── Access flag faults ──────────────────────────────────────────── */
        case DFSC_ACCESS_FLAG_L0: {
            case_print(DFSC_ACCESS_FLAG_L0);
            PANIC("Access flag faults not implemented!");
        } break;
        case DFSC_ACCESS_FLAG_L1: {
            case_print(DFSC_ACCESS_FLAG_L1);
            PANIC("Access flag faults not implemented!");
        } break;
        case DFSC_ACCESS_FLAG_L2: {
            case_print(DFSC_ACCESS_FLAG_L2);
            PANIC("Access flag faults not implemented!");
        } break;
        case DFSC_ACCESS_FLAG_L3: {
            case_print(DFSC_ACCESS_FLAG_L3);
            PANIC("Access flag faults not implemented!");
        } break;

        /* ── Permission faults ───────────────────────────────────────────── */
        case DFSC_PERMISSION_L0: {
            case_print(DFSC_PERMISSION_L0);
            terminate_task(3);
        } break;
        case DFSC_PERMISSION_L1: {
            case_print(DFSC_PERMISSION_L1);
            terminate_task(3);
        } break;
        case DFSC_PERMISSION_L2: {
            case_print(DFSC_PERMISSION_L2);
            terminate_task(3);
        } break;
        case DFSC_PERMISSION_L3: {
            case_print(DFSC_PERMISSION_L3);
            terminate_task(3);
        } break;

            /* ── Synchronous external aborts
             * ─────────────────────────────────── */
        case DFSC_SYNC_EXT_ABORT: {
            case_print(DFSC_SYNC_EXT_ABORT);
            PANIC("Synchronous external abort");
        } break;
        case DFSC_SYNC_TAG_CHECK: {
            case_print(DFSC_SYNC_TAG_CHECK);
            PANIC("Synchronous Tag Check fault");
        } break;
        case DFSC_SYNC_EXT_ABORT_TTW_Lm1: {
            case_print(DFSC_SYNC_EXT_ABORT_TTW_Lm1);
            PANIC("Synchronous external abort on TTW, level -1");
        } break;
        case DFSC_SYNC_EXT_ABORT_TTW_L0: {
            case_print(DFSC_SYNC_EXT_ABORT_TTW_L0);
            PANIC("Synchronous external abort on TTW, level 0");
        } break;
        case DFSC_SYNC_EXT_ABORT_TTW_L1: {
            case_print(DFSC_SYNC_EXT_ABORT_TTW_L1);
            PANIC("Synchronous external abort on TTW, level 1");
        } break;
        case DFSC_SYNC_EXT_ABORT_TTW_L2: {
            case_print(DFSC_SYNC_EXT_ABORT_TTW_L2);
            PANIC("Synchronous external abort on TTW, level 2");
        } break;
        case DFSC_SYNC_EXT_ABORT_TTW_L3: {
            case_print(DFSC_SYNC_EXT_ABORT_TTW_L3);
            PANIC("Synchronous external abort on TTW, level 3");
        } break;

        /* ── Parity / ECC errors ─────────────────────────────────────────── */
        case DFSC_SYNC_PARITY: {
            case_print(DFSC_SYNC_PARITY);
            PANIC("Synchronous parity/ECC error");
        } break;
        case DFSC_SYNC_PARITY_TTW_Lm1: {
            case_print(DFSC_SYNC_PARITY_TTW_Lm1);
            PANIC("Synchronous parity/ECC error on TTW, level -1");
        } break;
        case DFSC_SYNC_PARITY_TTW_L0: {
            case_print(DFSC_SYNC_PARITY_TTW_L0);
            PANIC("Synchronous parity/ECC error on TTW, level 0");
        } break;
        case DFSC_SYNC_PARITY_TTW_L1: {
            case_print(DFSC_SYNC_PARITY_TTW_L1);
            PANIC("Synchronous parity/ECC error on TTW, level 1");
        } break;
        case DFSC_SYNC_PARITY_TTW_L2: {
            case_print(DFSC_SYNC_PARITY_TTW_L2);
            PANIC("Synchronous parity/ECC error on TTW, level 2");
        } break;
        case DFSC_SYNC_PARITY_TTW_L3: {
            case_print(DFSC_SYNC_PARITY_TTW_L3);
            PANIC("Synchronous parity/ECC error on TTW, level 3");
        } break;

        /* ── Alignment ───────────────────────────────────────────────────── */
        case DFSC_ALIGNMENT: {
            case_print(DFSC_ALIGNMENT);
            PANIC("Alignment fault");
        } break;

        /* ── Granule Protection Faults ───────────────────────────────────── */
        case DFSC_GPF_TTW_Lm1: {
            case_print(DFSC_GPF_TTW_Lm1);
            PANIC("Granule Protection Fault on TTW, level -1");
        } break;
        case DFSC_GPF_TTW_L0: {
            case_print(DFSC_GPF_TTW_L0);
            PANIC("Granule Protection Fault on TTW, level 0");
        } break;
        case DFSC_GPF_TTW_L1: {
            case_print(DFSC_GPF_TTW_L1);
            PANIC("Granule Protection Fault on TTW, level 1");
        } break;
        case DFSC_GPF_TTW_L2: {
            case_print(DFSC_GPF_TTW_L2);
            PANIC("Granule Protection Fault on TTW, level 2");
        } break;
        case DFSC_GPF_TTW_L3: {
            case_print(DFSC_GPF_TTW_L3);
            PANIC("Granule Protection Fault on TTW, level 3");
        } break;
        case DFSC_GPF: {
            case_print(DFSC_GPF);
            PANIC("Granule Protection Fault");
        } break;

        /* ── Misc ────────────────────────────────────────────────────────── */
        case DFSC_TLB_CONFLICT: {
            case_print(DFSC_TLB_CONFLICT);
            PANIC("TLB conflict abort");
        } break;
        case DFSC_UNSUPPORTED_ATOMIC_HW: {
            case_print(DFSC_UNSUPPORTED_ATOMIC_HW);
            PANIC("Unsupported atomic hardware update");
        } break;
        case DFSC_IMPDEF_LOCKDOWN: {
            case_print(DFSC_IMPDEF_LOCKDOWN);
            PANIC("IMPLEMENTATION DEFINED fault: Lockdown");
        } break;
        case DFSC_IMPDEF_EXCL_ATOMIC: {
            case_print(DFSC_IMPDEF_EXCL_ATOMIC);
            PANIC("IMPLEMENTATION DEFINED fault: Unsupported Exclusive/Atomic");
        } break;


        default: {
            dbg_printf(DEBUG_TRACE, "DFSC unknown: %d\n", dabt_iss.DFSC);
        } break;
    }
}
