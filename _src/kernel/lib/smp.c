#include <arm/cpu.h>
#include <arm/sysregs/sysregs.h>
#include <arm/tfa/smccc.h>
#include <kernel/hardware.h>
#include <kernel/lib/smp.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>

bool wake_core(uint64_t core_id, uintptr_t entry_addr, uint64_t context)
{
    smccc_res_t res = _smc_call(
        PSCI_CPU_ON_FID64,
        core_id,
        entry_addr,
        context,
        0x0,
        0x0,
        0x0,
        0x0);

    if (res.x0 == 0)
        return true;

    return false;
}
