#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <arm/smccc/smc.h>
#include <arm/sysregs/sysregs.h>
#include <drivers/gicv3.h>
#include <kernel/init.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/userspace_embedded_examples.h>
#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "kernel/io/stdio.h"
#include "kernel/smp.h"
#include "kernel/time.h"


typedef struct {
    clock_t* clock;
    uint64_t nanosec;
} timer_test_t;

static void notify([[maybe_unused]] void* ctx)
{
    kprint("timer event received!\n\r");
}




// Main function of the kernel, called by the bootloader (/boot/boot.S)
noreturn void kernel_entry()
{
    if (get_cpuid() == 0) {
        if (!mm_kernel_is_relocated())
            kernel_early_init();
        else
            kernel_init();
    }

    kprint("Hello!\n\r");

#define SECOND 1e9

    for (size_t i = 0; i < 10; i++)
        timer_create_event_delta(HRTIMER, notify, NULL, SECOND * i);


    loop asm volatile("wfi");
}
