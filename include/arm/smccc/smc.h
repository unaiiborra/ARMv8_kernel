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
