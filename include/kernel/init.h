#pragma once

typedef void (*kernel_initcall_t)(void);

#define KERNEL_INITCALL(fn)                  \
    static kernel_initcall_t __initcall_##fn \
        __attribute__((section(".kernel_initcall"), used)) = fn;

#define KERNEL_CPU_INITCALL(fn)                 \
    static kernel_initcall_t __initcallcpu_##fn \
        __attribute__((section(".kernel_cpu_initcall"), used)) = fn;

void kernel_early_init(void);
void kernel_init(void);
void kernel_cpu_local_init();

bool kernel_is_initialized();
