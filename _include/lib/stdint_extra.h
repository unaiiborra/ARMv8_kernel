#pragma once

#include <stdint.h>

typedef enum {
    STDINT_INT8,
    STDINT_UINT8,
    STDINT_INT16,
    STDINT_UINT16,
    STDINT_INT32,
    STDINT_UINT32,
    STDINT_INT64,
    STDINT_UINT64,
} STDINT_TYPES;

/// Base representation of integer numbers
typedef enum {
    STDINT_BASE_REPR_DEC = 10,
    STDINT_BASE_REPR_HEX = 16,
    STDINT_BASE_REPR_BIN = 2,
    STDINT_BASE_REPR_OCT = 8,
} STDINT_BASE_REPR;


typedef union {
    int8_t int8;
    uint8_t uint8;

    int16_t int16;
    uint16_t uint16;

    int32_t int32;
    uint32_t uint32;

    int64_t int64;
    uint64_t uint64;
} STDINT_UNION;