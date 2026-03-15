#include <arm/exceptions/sync.h>
#include <arm/sysregs/sysregs.h>
#include <kernel/exception/handler.h>
#include <kernel/syscall.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/io/stdio.h"
#include "kernel/panic.h"
#include "kernel/scheduler.h"


// https://developer.arm.com/documentation/102412/0103/Handling-excepcions/Exception-handling-examples
// https://developer.arm.com/documentation/ddi0601/latest/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-


void exception_handler_sync(arm_exception_ctx* ectx)
{
    uint64_t esr_el1 = sysreg_read(esr_el1);
    esr_ec exception_class = ESR_EC(esr_el1);

    switch (exception_class) {
        case ESR_EC_UNKNOWN:
            PANIC("ESR_EC_UNKNOWN");
            break;

        case ESR_EC_WFI_WFE:
            dbg_print("exception: ESR_EC_WFI_WFE\n\r");
            break;

        case ESR_EC_MCR_MRC_CP15:
            dbg_print("exception: ESR_EC_MCR_MRC_CP15\n\r");
            break;

        case ESR_EC_MCRR_MRRC_CP15:
            dbg_print("exception: ESR_EC_MCRR_MRRC_CP15\n\r");
            break;

        case ESR_EC_MCR_MRC_CP14:
            dbg_print("exception: ESR_EC_MCR_MRC_CP14\n\r");
            break;

        case ESR_EC_LDC_STC:
            dbg_print("exception: ESR_EC_LDC_STC\n\r");
            break;

        case ESR_EC_FP_ASIMD_SVE:
            dbg_print("exception: ESR_EC_FP_ASIMD_SVE\n\r");
            break;

        case ESR_EC_PAUTH:
            dbg_print("exception: ESR_EC_PAUTH\n\r");
            break;

        case ESR_EC_LS64:
            dbg_print("exception: ESR_EC_LS64\n\r");
            break;

        case ESR_EC_MRRC_CP14:
            dbg_print("exception: ESR_EC_MRRC_CP14\n\r");
            break;

        case ESR_EC_BTI:
            dbg_print("exception: ESR_EC_BTI\n\r");
            break;

        case ESR_EC_ILLEGAL_STATE:
            dbg_print("exception: ESR_EC_ILLEGAL_STATE\n\r");
            break;

        case ESR_EC_SVC_AARCH32:
            dbg_print("exception: ESR_EC_SVC_AARCH32\n\r");
            break;

        case ESR_EC_SYSREG128:
            dbg_print("exception: ESR_EC_SYSREG128\n\r");
            break;

        case ESR_EC_SVC_AARCH64:
            dbg_print("exception: ESR_EC_SVC_AARCH64\n\r");
            scheduler_ectx_save(ectx);
            sysc64_dispatch(ectx);
            schedurer_ectx_restore(ectx);
            break;

        case ESR_EC_SYSREG_AARCH64:
            dbg_print("exception: ESR_EC_SYSREG_AARCH64\n\r");
            break;

        case ESR_EC_SVE:
            dbg_print("exception: ESR_EC_SVE\n\r");
            break;

        case ESR_EC_ERET:
            dbg_print("exception: ESR_EC_ERET\n\r");
            break;

        case ESR_EC_PAC_FAIL:
            dbg_print("exception: ESR_EC_PAC_FAIL\n\r");
            break;

        case ESR_EC_SME:
            dbg_print("exception: ESR_EC_SME\n\r");
            break;

        case ESR_EC_IABT_LOWER_EL:
            dbg_print("exception: ESR_EC_IABT_LOWER_EL\n\r");
            break;

        case ESR_EC_IABT_SAME_EL:
            dbg_print("exception: ESR_EC_IABT_SAME_EL\n\r");
            break;

        case ESR_EC_PC_ALIGNMENT:
            dbg_print("exception: ESR_EC_PC_ALIGNMENT\n\r");
            break;

        case ESR_EC_DABT_LOWER_EL:
            dbg_print("exception: ESR_EC_DABT_LOWER_EL\n\r");
            break;

        case ESR_EC_DABT_SAME_EL:
            dbg_print("exception: ESR_EC_DABT_SAME_EL\n\r");
            break;

        case ESR_EC_SP_ALIGNMENT:
            dbg_print("exception: ESR_EC_SP_ALIGNMENT\n\r");
            break;

        case ESR_EC_MOPS:
            dbg_print("exception: ESR_EC_MOPS\n\r");
            break;

        case ESR_EC_FP_TRAP_AARCH32:
            dbg_print("exception: ESR_EC_FP_TRAP_AARCH32\n\r");
            break;

        case ESR_EC_FP_TRAP_AARCH64:
            dbg_print("exception: ESR_EC_FP_TRAP_AARCH64\n\r");
            break;

        case ESR_EC_GCS:
            dbg_print("exception: ESR_EC_GCS\n\r");
            break;

        case ESR_EC_ILLEGAL_TINDEX:
            dbg_print("exception: ESR_EC_ILLEGAL_TINDEX\n\r");
            break;

        case ESR_EC_SERROR:
            dbg_print("exception: ESR_EC_SERROR\n\r");
            break;

        case ESR_EC_BRK_LOWER_EL:
            dbg_print("exception: ESR_EC_BRK_LOWER_EL\n\r");
            break;

        case ESR_EC_BRK_SAME_EL:
            dbg_print("exception: ESR_EC_BRK_SAME_EL\n\r");
            break;

        case ESR_EC_STEP_LOWER_EL:
            dbg_print("exception: ESR_EC_STEP_LOWER_EL\n\r");
            break;

        case ESR_EC_STEP_SAME_EL:
            dbg_print("exception: ESR_EC_STEP_SAME_EL\n\r");
            break;

        case ESR_EC_WATCH_LOWER_EL:
            dbg_print("exception: ESR_EC_WATCH_LOWER_EL\n\r");
            break;

        case ESR_EC_WATCH_SAME_EL:
            dbg_print("exception: ESR_EC_WATCH_SAME_EL\n\r");
            break;

        case ESR_EC_BKPT_AARCH32:
            dbg_print("exception: ESR_EC_BKPT_AARCH32\n\r");
            break;

        case ESR_EC_BRK_AARCH64:
            dbg_print("exception: ESR_EC_BRK_AARCH64\n\r");
            break;

        case ESR_EC_PROFILING:
            dbg_print("exception: ESR_EC_PROFILING\n\r");
            break;
    }
}
