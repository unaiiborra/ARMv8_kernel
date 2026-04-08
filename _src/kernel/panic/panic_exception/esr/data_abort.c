#include <kernel/io/stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "esr.h"


typedef struct {
    uint32_t DFSC;   /* [5:0]   */
    uint32_t WnR;    /* [6]     */
    uint32_t S1PTW;  /* [7]     */
    uint32_t CM;     /* [8]     */
    uint32_t EA;     /* [9]     */
    uint32_t FnV;    /* [10]    */
    uint32_t b12_11; /* [12:11] */
    uint32_t b14;    /* [14]    */
    uint32_t b15;    /* [15]    */
    uint32_t b20_16; /* [20:16] */
    uint32_t SSE;    /* [21]    */
    uint32_t SAS;    /* [23:22] */
    uint32_t ISV;    /* [24]    */
} data_abort_iss;


static data_abort_iss data_abort_iss_new(uint32_t iss)
{
    data_abort_iss d;

    d.DFSC   = iss & 0x3fu;         /* bits [5:0]   */
    d.WnR    = (iss >> 6) & 0x1u;   /* bit  [6]     */
    d.S1PTW  = (iss >> 7) & 0x1u;   /* bit  [7]     */
    d.CM     = (iss >> 8) & 0x1u;   /* bit  [8]     */
    d.EA     = (iss >> 9) & 0x1u;   /* bit  [9]     */
    d.FnV    = (iss >> 10) & 0x1u;  /* bit  [10]    */
    d.b12_11 = (iss >> 11) & 0x3u;  /* bits [12:11] */
    d.b14    = (iss >> 14) & 0x1u;  /* bit  [14]    */
    d.b15    = (iss >> 15) & 0x1u;  /* bit  [15]    */
    d.b20_16 = (iss >> 16) & 0x1fu; /* bits [20:16] */
    d.SSE    = (iss >> 21) & 0x1u;  /* bit  [21]    */
    d.SAS    = (iss >> 22) & 0x3u;  /* bits [23:22] */
    d.ISV    = (iss >> 24) & 0x1u;  /* bit  [24]    */

    return d;
}


static const char* dfsc_to_string(uint32_t DSFC)
{
    switch (DSFC) {
        case 0b000000:
            return "Address size fault, level 0 of translation or translation "
                   "table base "
                   "register.";
        case 0b000001:
            return "Address size fault, level 1.";
        case 0b000010:
            return "Address size fault, level 2.";
        case 0b000011:
            return "Address size fault, level 3.";

        case 0b000100:
            return "Translation fault, level 0.";
        case 0b000101:
            return "Translation fault, level 1.";
        case 0b000110:
            return "Translation fault, level 2.";
        case 0b000111:
            return "Translation fault, level 3.";

        case 0b001000:
            return "Access flag fault, level 0.";
        case 0b001001:
            return "Access flag fault, level 1.";
        case 0b001010:
            return "Access flag fault, level 2.";
        case 0b001011:
            return "Access flag fault, level 3.";

        case 0b001100:
            return "Permission fault, level 0.";
        case 0b001101:
            return "Permission fault, level 1.";
        case 0b001110:
            return "Permission fault, level 2.";
        case 0b001111:
            return "Permission fault, level 3.";

        case 0b010000:
            return "Synchronous External abort, not on translation table walk "
                   "or hardware "
                   "update of translation table.";
        case 0b010001:
            return "Synchronous Tag Check Fault.";

        case 0b010010:
            return "Synchronous External abort on translation table walk or "
                   "hardware update of "
                   "translation table, level -2.";
        case 0b010011:
            return "Synchronous External abort on translation table walk or "
                   "hardware update of "
                   "translation table, level -1.";
        case 0b010100:
            return "Synchronous External abort on translation table walk or "
                   "hardware update of "
                   "translation table, level 0.";
        case 0b010101:
            return "Synchronous External abort on translation table walk or "
                   "hardware update of "
                   "translation table, level 1.";
        case 0b010110:
            return "Synchronous External abort on translation table walk or "
                   "hardware update of "
                   "translation table, level 2.";
        case 0b010111:
            return "Synchronous External abort on translation table walk or "
                   "hardware update of "
                   "translation table, level 3.";

        case 0b011000:
            return "Synchronous parity or ECC error on memory access, not on "
                   "translation table "
                   "walk.";
        case 0b011011:
            return "Synchronous parity or ECC error on memory access on "
                   "translation table walk "
                   "or hardware update of translation table, level -1.";
        case 0b011100:
            return "Synchronous parity or ECC error on memory access on "
                   "translation table walk "
                   "or hardware update of translation table, level 0.";
        case 0b011101:
            return "Synchronous parity or ECC error on memory access on "
                   "translation table walk "
                   "or hardware update of translation table, level 1.";
        case 0b011110:
            return "Synchronous parity or ECC error on memory access on "
                   "translation table walk "
                   "or hardware update of translation table, level 2.";
        case 0b011111:
            return "Synchronous parity or ECC error on memory access on "
                   "translation table walk "
                   "or hardware update of translation table, level 3.";

        case 0b100001:
            return "Alignment fault.";

        case 0b100010:
            return "Granule Protection Fault on translation table walk or "
                   "hardware update of "
                   "translation table, level -2.";
        case 0b100011:
            return "Granule Protection Fault on translation table walk or "
                   "hardware update of "
                   "translation table, level -1.";
        case 0b100100:
            return "Granule Protection Fault on translation table walk or "
                   "hardware update of "
                   "translation table, level 0.";
        case 0b100101:
            return "Granule Protection Fault on translation table walk or "
                   "hardware update of "
                   "translation table, level 1.";
        case 0b100110:
            return "Granule Protection Fault on translation table walk or "
                   "hardware update of "
                   "translation table, level 2.";
        case 0b100111:
            return "Granule Protection Fault on translation table walk or "
                   "hardware update of "
                   "translation table, level 3.";

        case 0b101000:
            return "Granule Protection Fault, not on translation table walk or "
                   "hardware update "
                   "of translation table.";
        case 0b101001:
            return "Address size fault, level -1.";
        case 0b101010:
            return "Translation fault, level -2.";
        case 0b101011:
            return "Translation fault, level -1.";
        case 0b101100:
            return "Address Size fault, level -2.";

        case 0b110000:
            return "TLB conflict abort.";
        case 0b110001:
            return "Unsupported atomic hardware update fault.";
        case 0b110010:
            return "PLB conflict abort.";
        case 0b110100:
            return "IMPLEMENTATION DEFINED fault (Lockdown).";
        case 0b110101:
            return "IMPLEMENTATION DEFINED fault (Unsupported Exclusive or "
                   "Atomic access).";

        default:
            return "Unknown DFSC value.";
    }
}


void print_data_abort_info(uint64_t iss, uint64_t)
{
    data_abort_iss f = data_abort_iss_new((uint32_t)iss);

    fkprintf(
        IO_STDPANIC,
        "\tDFSC (Data Fault Status Code): %s\n",
        dfsc_to_string(f.DFSC));

    fkprintf(
        IO_STDPANIC,
        "\t\tWnR (Write not Read): %s\n",
        f.WnR
            ? "Abort caused by an instruction writing to a memory location."
            : "Abort caused by an instruction reading from a memory location.");

    fkprintf(
        IO_STDPANIC,
        "\t\tS1PTW (Stage 2 fault on Stage 1 translation table walk): %s\n",
        f.S1PTW ? "Fault on the stage 2 translation of an access for a stage 1 "
                  "translation table walk."
                : "Fault not on a stage 2 translation for a stage 1 "
                  "translation table walk.");

    fkprintf(
        IO_STDPANIC,
        "\t\tCM (Cache maintenance): %s\n",
        f.CM ? "The Data Abort was generated by either the execution of a "
               "cache maintenance "
               "instruction or by a synchronous fault on the execution of an "
               "address "
               "translation instruction."
             : "The Data Abort was not generated by the execution of one of "
               "the System "
               "instructions identified in the description of value 1.");

    fkprintf(
        IO_STDPANIC,
        "\t\tEA (External abort type): %s\n",
        f.EA ? "External abort type. This bit can provide an IMPLEMENTATION "
               "DEFINED "
               "classification of External aborts."
             : "For any abort other than an External abort this bit returns a "
               "value of 0.");

    fkprintf(
        IO_STDPANIC,
        "\t\tFnV (FAR not Valid): %s\n",
        f.FnV ? "FAR is not valid, and holds an UNKNOWN value."
              : "FAR is valid.");

    fkprintf(IO_STDPANIC, "\t\tBits[12:11]: %b\n", f.b12_11);

    fkprintf(IO_STDPANIC, "\t\tBit[14]: %b\n", f.b14);
    fkprintf(IO_STDPANIC, "\t\tBit[15]: %b\n", f.b15);

    fkprintf(IO_STDPANIC, "\t\tBits[20:16]: %b\n", f.b20_16);

    fkprintf(
        IO_STDPANIC,
        "\t\tSSE (Syndrome Sign Extend): %s\n",
        f.SSE ? "Data item must be sign-extended."
              : "Sign-extension not required.");

    switch (f.SAS) {
        case 0b00:
            fkprint(IO_STDPANIC, "\t\tSAS (Syndrome Access Size): Byte\n");
            break;
        case 0b01:
            fkprint(IO_STDPANIC, "\t\tSAS (Syndrome Access Size): Halfword\n");
            break;
        case 0b10:
            fkprint(IO_STDPANIC, "\t\tSAS (Syndrome Access Size): Word\n");
            break;
        case 0b11:
            fkprint(IO_STDPANIC, "\t\tSAS (Syndrome Access Size): Doubleword\n");
            break;
    }

    fkprintf(
        IO_STDPANIC,
        "\t\tISV (Instruction Syndrome Valid): %s\n",
        f.ISV ? "ISS[23:14] hold a valid instruction syndrome."
              : "No valid instruction syndrome. ISS[23:14] are RES0.");
}
