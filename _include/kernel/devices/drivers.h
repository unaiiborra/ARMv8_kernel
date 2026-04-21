#pragma once

#include <lib/mem.h>
#include <stdint.h>


[[deprecated("register a device_t instead")]] typedef struct {
    uintptr_t base;
    void*     state;
} driver_handle;

extern const driver_handle GIC_DRIVER;

extern driver_handle UART1_DRIVER;
extern driver_handle UART2_DRIVER;
extern driver_handle UART3_DRIVER;
extern driver_handle UART4_DRIVER;

extern driver_handle TMU_DRIVER;

extern driver_handle AGT0_DRIVER;



typedef struct {
    vuintptr_t   base;
    void** const state;
} driver_handle_t;


typedef enum {
    TRIGGER_LEVEL_SENSITIVE = 0,
    TRIGGER_EDGE_TRIGGERED  = 1
} irq_ctrl_ops_trigger_t;

typedef struct {
    int32_t (*init)(driver_handle_t handle);
    int32_t (*init_cpu)(driver_handle_t handle, uint32_t target_cpu);

    int32_t (*irq_enable)(driver_handle_t handle, uint32_t irq);
    int32_t (*irq_disable)(driver_handle_t handle, uint32_t irq);

    int32_t (*irq_eoi)(driver_handle_t handle, uint32_t irq);

    uint32_t (*irq_ack)(driver_handle_t handle);

    int32_t (*irq_set_priority)(
        driver_handle_t handle,
        uint32_t        irq,
        uint8_t         priority);

    int32_t (*irq_set_target)(
        driver_handle_t handle,
        uint32_t        irq,
        uint32_t        target_cpu);

    int32_t (*irq_set_trigger)(
        driver_handle_t        handle,
        uint32_t               irq,
        irq_ctrl_ops_trigger_t trigger);
} irq_ctrl_ops_t;
