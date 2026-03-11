#include "esr.h"

#include <arm/sysregs/sysregs.h>
#include <kernel/io/stdio.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>

#include "../panic_exception_handlers.h"

/// https://developer.arm.com/documentation/111107/2025-09/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-

typedef enum {
    ESR_EC_UNKNOWN = 0b000000,
    ESR_EC_WFI_WFE = 0b000001,
    ESR_EC_MCR_MRC_CP15 = 0b000011,
    ESR_EC_MCRR_MRRC_CP15 = 0b000100,
    ESR_EC_MCR_MRC_CP14 = 0b000101,
    ESR_EC_LDC_STC = 0b000110,
    ESR_EC_FP_ASIMD_SVE = 0b000111,
    ESR_EC_PAUTH = 0b001001,
    ESR_EC_LS64 = 0b001010,
    ESR_EC_MRRC_CP14 = 0b001100,
    ESR_EC_BTI = 0b001101,
    ESR_EC_ILLEGAL_STATE = 0b001110,

    ESR_EC_SVC_AARCH32 = 0b010001,
    ESR_EC_SYSREG128 = 0b010100,
    ESR_EC_SVC_AARCH64 = 0b010101,
    ESR_EC_SYSREG_AARCH64 = 0b011000,
    ESR_EC_SVE = 0b011001,
    ESR_EC_ERET = 0b011010,
    ESR_EC_PAC_FAIL = 0b011100,
    ESR_EC_SME = 0b011101,

    ESR_EC_IABT_LOWER_EL = 0b100000,
    ESR_EC_IABT_SAME_EL = 0b100001,
    ESR_EC_PC_ALIGNMENT = 0b100010,
    ESR_EC_DABT_LOWER_EL = 0b100100,
    ESR_EC_DABT_SAME_EL = 0b100101,
    ESR_EC_SP_ALIGNMENT = 0b100110,
    ESR_EC_MOPS = 0b100111,

    ESR_EC_FP_TRAP_AARCH32 = 0b101000,
    ESR_EC_FP_TRAP_AARCH64 = 0b101100,
    ESR_EC_GCS = 0b101101,
    ESR_EC_ILLEGAL_TINDEX = 0b101110,
    ESR_EC_SERROR = 0b101111,

    ESR_EC_BRK_LOWER_EL = 0b110000,
    ESR_EC_BRK_SAME_EL = 0b110001,
    ESR_EC_STEP_LOWER_EL = 0b110010,
    ESR_EC_STEP_SAME_EL = 0b110011,
    ESR_EC_WATCH_LOWER_EL = 0b110100,
    ESR_EC_WATCH_SAME_EL = 0b110101,

    ESR_EC_BKPT_AARCH32 = 0b111000,
    ESR_EC_BRK_AARCH64 = 0b111100,
    ESR_EC_PROFILING = 0b111101,
} esr_ec;


static const char* ec_msg(esr_ec ec)
{
    switch (ec) {
        case ESR_EC_UNKNOWN:
            return "\tUnknown reason";
        case ESR_EC_WFI_WFE:
            return "\tTrapped WFI/WFE instruction";
        case ESR_EC_MCR_MRC_CP15:
            return "\tTrapped MCR/MRC (CP15)";
        case ESR_EC_MCRR_MRRC_CP15:
            return "\tTrapped MCRR/MRRC (CP15)";
        case ESR_EC_MCR_MRC_CP14:
            return "\tTrapped MCR/MRC (CP14)";
        case ESR_EC_LDC_STC:
            return "\tTrapped LDC/STC";
        case ESR_EC_FP_ASIMD_SVE:
            return "\tTrapped FP/ASIMD/SVE access";
        case ESR_EC_PAUTH:
            return "\tTrapped Pointer Authentication instruction";
        case ESR_EC_LS64:
            return "\tTrapped LS64 instruction";
        case ESR_EC_MRRC_CP14:
            return "\tTrapped MRRC (CP14)";
        case ESR_EC_BTI:
            return "\tBranch Target Identification fault";
        case ESR_EC_ILLEGAL_STATE:
            return "\tIllegal execution state";
        case ESR_EC_SVC_AARCH32:
            return "\tSVC from AArch32";
        case ESR_EC_SYSREG128:
            return "\tTrapped 128-bit system instruction";
        case ESR_EC_SVC_AARCH64:
            return "\tSVC from AArch64";
        case ESR_EC_SYSREG_AARCH64:
            return "\tTrapped system register access (AArch64)";
        case ESR_EC_SVE:
            return "\tTrapped SVE access";
        case ESR_EC_ERET:
            return "\tTrapped ERET/ERETAA/ERETAB";
        case ESR_EC_PAC_FAIL:
            return "\tPAC failure";
        case ESR_EC_SME:
            return "\tTrapped SME access";
        case ESR_EC_IABT_LOWER_EL:
            return "\tInstruction Abort (lower EL)";
        case ESR_EC_IABT_SAME_EL:
            return "\tInstruction Abort (same EL)";
        case ESR_EC_PC_ALIGNMENT:
            return "\tPC alignment fault";
        case ESR_EC_DABT_LOWER_EL:
            return "\tData Abort (lower EL)";
        case ESR_EC_DABT_SAME_EL:
            return "\tData Abort (same EL)";
        case ESR_EC_SP_ALIGNMENT:
            return "\tSP alignment fault";
        case ESR_EC_MOPS:
            return "\tMemory Operation exception (MOPS)";
        case ESR_EC_FP_TRAP_AARCH32:
            return "\tTrapped FP exception (AArch32)";
        case ESR_EC_FP_TRAP_AARCH64:
            return "\tTrapped FP exception (AArch64)";
        case ESR_EC_GCS:
            return "\tGCS exception";
        case ESR_EC_ILLEGAL_TINDEX:
            return "\tIllegal TIndex change";
        case ESR_EC_SERROR:
            return "\tSError exception";
        case ESR_EC_BRK_LOWER_EL:
            return "\tBreakpoint (lower EL)";
        case ESR_EC_BRK_SAME_EL:
            return "\tBreakpoint (same EL)";
        case ESR_EC_STEP_LOWER_EL:
            return "\tSoftware step (lower EL)";
        case ESR_EC_STEP_SAME_EL:
            return "\tSoftware step (same EL)";
        case ESR_EC_WATCH_LOWER_EL:
            return "\tWatchpoint (lower EL)";
        case ESR_EC_WATCH_SAME_EL:
            return "\tWatchpoint (same EL)";
        case ESR_EC_BKPT_AARCH32:
            return "\tBKPT instruction (AArch32)";
        case ESR_EC_BRK_AARCH64:
            return "\tBRK instruction (AArch64)";
        case ESR_EC_PROFILING:
            return "\tProfiling exception";
        default:
            return "\tReserved or unknown value";
    }
}


void print_esr(exception_reason_sysregs* r, panic_exception_type type)
{
#define ESR_EC(esr) (((esr) >> 26) & 0x3f)
#define ESR_IL(esr) (((esr) >> 25) & 1)
#define ESR_ISS(esr) ((esr) & 0x1ffffff)
#define ESR_ISS2(esr) (((esr) >> 32) & 0xffffff)


    const uint64_t esr = r->esr;

    fkprintf(
        IO_STDPANIC,
        "[ESR_EL%d "
        "(https://developer.arm.com/documentation/111107/2025-12/"
        "AArch64-Registers/"
        "ESR-EL1--Exception-Syndrome-Register--EL1-)]\n",
        (int)_ARM_currentEL());
    fkprintf(IO_STDPANIC, "\traw: %p\n\t(%b)\n", esr, esr);

    /* IRQ / FIQ: ESR no es diagnóstico */
    if (type == PANIC_EXCEPTION_TYPE_IRQ || type == PANIC_EXCEPTION_TYPE_FIQ) {
        fkprint(
            IO_STDPANIC,
            "\tnote: asynchronous interrupt, esr not relevant\n");
        return;
    }


    const esr_ec ec = ESR_EC(esr);
    const uint64_t il = ESR_IL(esr);
    const uint64_t iss = ESR_ISS(esr);
    const uint64_t iss2 = ESR_ISS2(esr);


    if (type == PANIC_EXCEPTION_TYPE_SYNC) {
        fkprintf(
            IO_STDPANIC,
            "\tIL (instruction lenght): %s\n",
            il ? "32 bit" : "16 bit");
        fkprintf(IO_STDPANIC, "\tEC (exception class): %s\n", ec_msg(ec));

        switch (ec) {
            case ESR_EC_UNKNOWN:
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
                print_data_abort_info(iss, iss2);
                break;
            case ESR_EC_DABT_SAME_EL:
                print_data_abort_info(iss, iss2);
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


    if (type == PANIC_EXCEPTION_TYPE_SERROR) {
    }
}
