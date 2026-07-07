#include <arm/exceptions/handlers/handlers_macros.h>
#include <kernel/io/stdio.h>
#include <stdint.h>

#include "arm/sysregs/sysregs.h"
#include "kernel/hardware.h"
#include "kernel/smp.h"
#include "lib/align.h"
#include "lib/branch.h"

// See:
// https://community.arm.com/support-forums/f/architectures-and-processors-forum/48088/osv-guest-encountering-ec---unknown-reason-sync-exception-esr-0x2000000-on-raspberry-pi-4b-host-with-kvm-on
// https://github.com/cloudius-systems/osv/issues/1100
#define MAX_UNKNOWN_ESR 50000000

static uint64_t last_unknown_esr[NUM_CPUS]  = {0x0};
static uint64_t unknown_esr_count[NUM_CPUS] = {0};

void el1_cur_spx_sync_handler(arm_ctx_t*)
{
    if (sysreg_read(esr_el1) == 0x2000000) {
        cpuid_t  cpuid           = get_cpuid();
        uint64_t current_unknown = sysreg_read(elr_el1);
        uint64_t cache_line      = align_down(current_unknown, CACHE_LINE);

        asm volatile("dc cvau, %0" ::"r"(cache_line));
        asm volatile("dsb ish");
        asm volatile("ic ialluis");
        asm volatile("dsb ish");
        asm volatile("isb");

        if (current_unknown != last_unknown_esr[cpuid]) {
            unknown_esr_count[cpuid] = 1;
            last_unknown_esr[cpuid]  = current_unknown;
        }
        else
            unknown_esr_count[cpuid]++;

        if (unknown_esr_count[cpuid] % 100000 == 0) {
            dbg_printf(
                DEBUG_TRACE,
                "[CORE %d] unknown esr: %x -> %l\n\r",
                get_cpuid(),
                last_unknown_esr[cpuid],
                unknown_esr_count[cpuid]);
        }

        if (likely(unknown_esr_count[cpuid] < MAX_UNKNOWN_ESR))
            return;
    }

    exception_panic(
        "el1_cur_spx_sync exception",
        select_src_enum_("cur", "spx"),
        select_exception_type_enum_("sync"));
}
