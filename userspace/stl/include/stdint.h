#pragma once

#ifdef _STD_TYPES_T
#    error "Used c stdlib stdint implementation instead of baremetal custom stdint"
#else
#    define _STD_TYPES_T
#endif

#define STDINT

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
	STDINT_BASE_REPR_DEC	= 10,
	STDINT_BASE_REPR_HEX	= 16,
	STDINT_BASE_REPR_BIN	= 2,
	STDINT_BASE_REPR_OCT	= 8,
} STDINT_BASE_REPR;

typedef signed char int8;
typedef unsigned char uint8;

typedef signed short int16;
typedef unsigned short uint16;

typedef signed int int32;
typedef unsigned int uint32;

typedef signed long long int64;
typedef unsigned long long uint64;



#define INT8_MIN      (-128)
#define INT8_MAX      127
#define UINT8_MAX     255U

#define INT16_MIN     (-32768)
#define INT16_MAX     32767
#define UINT16_MAX    65535U

#define INT32_MIN     (-2147483647 - 1)
#define INT32_MAX     2147483647
#define UINT32_MAX    4294967295U

#define INT64_MIN     (-9223372036854775807LL - 1LL)
#define INT64_MAX     9223372036854775807LL
#define UINT64_MAX    18446744073709551615ULL


// TODO: acabar estos
#define INTPTR_MIN     LONG_MIN
#define INTPTR_MAX     LONG_MAX
#define UINTPTR_MAX    ULONG_MAX

#define INTMAX_MIN     INT64_MIN
#define INTMAX_MAX     INT64_MAX
#define UINTMAX_MAX    UINT64_MAX

typedef union {
	int8	int8;
	uint8	uint8;

	int16	int16;
	uint16	uint16;

	int32	int32;
	uint32	uint32;

	int64	int64;
	uint64	uint64;
} STDINT_UNION;
