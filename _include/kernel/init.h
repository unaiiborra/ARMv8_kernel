#pragma once

typedef void (*kernel_initcall_t)(void);

typedef enum {
    KERNEL_INITCALL_STAGE0 = 0,
    KERNEL_INITCALL_STAGE1,
    KERNEL_INITCALL_STAGE2,
} KERNEL_INITCALL_STAGE;

#define SET_KERNEL_INITCALL_STAGE0(fn)       \
    static kernel_initcall_t __initcall_##fn \
        __attribute__((section(".kernel_init_stage0"), used)) = fn;

#define SET_KERNEL_INITCALL_STAGE1(fn)       \
    static kernel_initcall_t __initcall_##fn \
        __attribute__((section(".kernel_init_stage1"), used)) = fn;

#define SET_KERNEL_INITCALL_STAGE2(fn)       \
    static kernel_initcall_t __initcall_##fn \
        __attribute__((section(".kernel_init_stage2"), used)) = fn;

#define KERNEL_INITCALL(fn, KERNEL_INITCALL_STAGE) \
    SET_##KERNEL_INITCALL_STAGE(fn)


void kernel_early_init(void);
void kernel_init(void);

bool kernel_is_initialized();