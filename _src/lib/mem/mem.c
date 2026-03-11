#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/panic.h"


size_t pa_supported_bits()
{
    // https://developer.arm.com/documentation/ddi0601/2025-12/AArch64-Registers/ID-AA64MMFR0-EL1--AArch64-Memory-Model-Feature-Register-0
    extern uint64_t _ID_AA64MMFR0_EL1_read(void);

    uint64_t PARange = _ID_AA64MMFR0_EL1_read() & 0xFULL;

    switch (PARange) {
        case 0b0000: // 32-bit PA (4GB)
            return 32;

        case 0b0001: // 36-bit PA (64GB)
            return 36;

        case 0b0010: // 40-bit PA (1TB)
            return 40;

        case 0b0011: // 42-bit PA (4TB)
            return 42;

        case 0b0100: // 44-bit PA (16TB)
            return 44;

        case 0b0101: // 48-bit PA (256TB)
            return 48;

        case 0b0110: // 52-bit PA (4PB) – requires FEAT_LPA
            return 52;

        case 0b0111: // 56-bit PA (64PB) – requires FEAT_D128
            return 56;
    }

    PANIC();
}
