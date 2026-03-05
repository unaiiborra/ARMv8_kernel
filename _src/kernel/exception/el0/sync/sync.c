#include "sync.h"

#include <arm/sysregs/sysregs.h>
#include <kernel/exception/handler.h>
#include <lib/stdint.h>

#include "exception_class/exception_class.h"

// https://developer.arm.com/documentation/102412/0103/Handling-excepcions/Exception-handling-examples
// https://developer.arm.com/documentation/ddi0601/latest/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-


void exception_handler_sync()
{
    uint64 esr_el1 = _ARM_ESR_EL1();
    esr_ec exception_class = ESR_EC(esr_el1);

    switch (exception_class) {
        case ESR_EC_UNKNOWN:
#ifdef GDB
            // gdb when performing a si or ni step over an svc call, iterferes
            // over the ESR because it causes a debug exception. Because of this
            // the received esr will be unknown
            svc_aarch64_handler();
#endif
            break;
        case ESR_EC_WFI_WFE:
            break;
        case ESR_EC_MCR_MRC_CP15:
            break;
        case ESR_EC_MCRR_MRRC_CP15:
            break;
        case ESR_EC_MCR_MRC_CP14:
            break;
        case ESR_EC_LDC_STC:
            break;
        case ESR_EC_FP_ASIMD_SVE:
            break;
        case ESR_EC_PAUTH:
            break;
        case ESR_EC_LS64:
            break;
        case ESR_EC_MRRC_CP14:
            break;
        case ESR_EC_BTI:
            break;
        case ESR_EC_ILLEGAL_STATE:
            break;
        case ESR_EC_SVC_AARCH32:
            break;
        case ESR_EC_SYSREG128:
            break;
        case ESR_EC_SVC_AARCH64:
            svc_aarch64_handler();
            break;
        case ESR_EC_SYSREG_AARCH64:
            break;
        case ESR_EC_SVE:
            break;
        case ESR_EC_ERET:
            break;
        case ESR_EC_PAC_FAIL:
            break;
        case ESR_EC_SME:
            break;
        case ESR_EC_IABT_LOWER_EL:
            break;
        case ESR_EC_IABT_SAME_EL:
            break;
        case ESR_EC_PC_ALIGNMENT:
            break;
        case ESR_EC_DABT_LOWER_EL:
            break;
        case ESR_EC_DABT_SAME_EL:
            break;
        case ESR_EC_SP_ALIGNMENT:
            break;
        case ESR_EC_MOPS:
            break;
        case ESR_EC_FP_TRAP_AARCH32:
            break;
        case ESR_EC_FP_TRAP_AARCH64:
            break;
        case ESR_EC_GCS:
            break;
        case ESR_EC_ILLEGAL_TINDEX:
            break;
        case ESR_EC_SERROR:
            break;
        case ESR_EC_BRK_LOWER_EL:
            break;
        case ESR_EC_BRK_SAME_EL:
            break;
        case ESR_EC_STEP_LOWER_EL:
            break;
        case ESR_EC_STEP_SAME_EL:
            break;
        case ESR_EC_WATCH_LOWER_EL:
            break;
        case ESR_EC_WATCH_SAME_EL:
            break;
        case ESR_EC_BKPT_AARCH32:
            break;
        case ESR_EC_BRK_AARCH64:
            break;
        case ESR_EC_PROFILING:
            break;
    }
}
