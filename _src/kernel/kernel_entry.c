#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <arm/smccc/smc.h>
#include <arm/sysregs/sysregs.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <drivers/gicv3.h>
#include <drivers/tmu/tmu.h>
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

#include "kernel/devices/device.h"
#include "kernel/devices/driver_ops/clocksource.h"
#include "kernel/devices/driver_ops/serial.h"
#include "kernel/devices/driver_ops/timer.h"
#include "kernel/io/stdio.h"
#include "kernel/smp.h"


typedef struct {
    uint32_t        calln;
    const device_t* timer;
    const device_t* clocksource;
    uint64_t        freq;
} timer_test_t;


static void notify(void* time)
{
    timer_test_t*      t                  = time;
    driver_handle_t    timer_handle       = device_get_driver_handle(t->timer);
    const timer_ops_t* timer_ops          = get_timer_ops(t->timer);
    driver_handle_t    clocksource_handle = device_get_driver_handle(
        t->clocksource);
    const clocksource_ops_t* clocksource_ops = get_clocksource_ops(
        t->clocksource);

    kprintf("\n\rtimer irq calln: %d", t->calln++);

    timer_ops->irq_notify_tick(
        timer_handle,
        t->freq + clocksource_ops->get_ticks(clocksource_handle),
        notify,
        time);
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

    const device_t*     uart        = device_get_primary(DEVICE_CLASS_SERIAL);
    driver_handle_t     uart_handle = device_get_driver_handle(uart);
    const serial_ops_t* uart_ops    = get_serial_ops(uart);

    uart_ops->init(uart_handle);
    uart_ops->set_baud(uart_handle, 115200, 12000000);
    uart_ops->irq_enable(uart_handle);

    const device_t* clocksource = device_get_primary(DEVICE_CLASS_CLOCKSOURCE);
    driver_handle_t clocksource_handle = device_get_driver_handle(clocksource);
    const clocksource_ops_t* clocksource_ops = get_clocksource_ops(clocksource);

    const device_t*    timer        = device_get_primary(DEVICE_CLASS_TIMER);
    driver_handle_t    timer_handle = device_get_driver_handle(timer);
    const timer_ops_t* timer_ops    = get_timer_ops(timer);

    uint64_t freq = clocksource_ops->get_freq_hz(clocksource_handle);

    timer_test_t ctx = (timer_test_t) {
        .calln       = 0,
        .timer       = timer,
        .clocksource = clocksource,
        .freq        = freq,
    };

    timer_ops->init(timer_handle);
    timer_ops->irq_notify_tick(
        timer_handle,
        freq + clocksource_ops->get_ticks(clocksource_handle),
        notify,
        &ctx);

    loop asm volatile("wfi");
}
