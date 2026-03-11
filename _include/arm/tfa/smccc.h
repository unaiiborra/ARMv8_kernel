#pragma once

#include <stddef.h>
#include <stdint.h>


typedef struct {
    int64_t x0;
    int64_t x1;
    int64_t x2;
    int64_t x3;
} smccc_res_t;

extern smccc_res_t _smc_call(
    uint64_t fid,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6,
    uint64_t arg7);

#define ARM_SMCCC_VERSION_FID 0x80000000u
#define SMCCC_ARCH_FEATURES_FID 0x80000001u
#define PSCI_VERSION_FID 0x84000000u
#define PSCI_SYSTEM_OFF_FID 0x84000008u
#define PSCI_CPU_ON_FID64 0x84000003u
#define PSCI_VERSION 0x84000000u

uint32_t tfa_get_smccc_version(void);
