#pragma once

#include <arm/cpu.h>
#include <arm/smccc/smc.h>
#include <lib/mem.h>
#include <lib/stdattribute.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>


typedef enum {
    PSCI_SUCCESS            = 0,
    PSCI_NOT_SUPPORTED      = -1,
    PSCI_INVALID_PARAMETERS = -2,
    PSCI_DENIED             = -3,
    PSCI_ALREADY_ON         = -4,
    PSCI_ON_PENDING         = -5,
    PSCI_INTERNAL_FAILURE   = -6,
    PSCI_NOT_PRESENT        = -7,
    PSCI_DISABLED           = -8,
    PSCI_INVALID_ADDRESS    = -9,
} psci_return_code;

typedef struct {
    uint16_t minor, major;
} psci_version_res;


psci_version_res psci_version();



noreturn void psci_cpu_off();


typedef void (*psci_cpu_on_entry)(size_t context_id);

psci_return_code psci_cpu_on(
    arm_cpu_affinity  target_cpu,
    psci_cpu_on_entry phys_entry_point_address,
    size_t            context_id // passed as x0 on the waken core
);


noreturn void psci_system_off();