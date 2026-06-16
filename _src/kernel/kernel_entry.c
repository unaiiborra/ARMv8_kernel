#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <arm/smccc/smc.h>
#include <arm/sysregs/sysregs.h>
#include <drivers/gicv3.h>
#include <kernel/embedded_binary.h>
#include <kernel/init.h>
#include <kernel/io/stdio.h>
#include <lib/data_structures/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "kernel/devices/device.h"
#include "kernel/io/vfs_serial.h"
#include "kernel/mm/elf.h"
#include "kernel/smp.h"
#include "kernel/vfs.h"

// Main function of the kernel, called by the bootloader (/boot/boot.S)
noreturn void kernel_entry()
{
    if (get_cpuid() == 0) {
        if (!mm_kernel_is_relocated())
            kernel_early_init();
        else
            kernel_init();
    }

    fd_table_t table = FD_TABLE_INIT;
    vfs_serial_bind_stdio(&table, device_get_primary(DEVICE_CLASS_SERIAL)->uid);

    const char* hello = "Hello from stdout!\n\r";
    vfs_write(&table, 1, (const uint8_t*)hello, strlen(hello));

    printf("Hello from the %s!\n\r", "kernel");

    elf_load_result elf_res;
    uintptr_t       shell_entry;

    task_t* shell = task_new("shell");

    elf_res = elf_load(
        shell,
        EMBEDDED_BINARY(shell_elf),
        EMBEDDED_BINARY_SIZE(shell_elf),
        &shell_entry);
    ASSERT(elf_res == ELF_LOAD_OK);

    schedule_ready_thread(shell, shell_entry);

    scheduler_loop_cpu_enter();

    loop asm volatile("wfi");
}
