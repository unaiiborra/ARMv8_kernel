#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef TEST

#    include <kernel/panic.h>

static inline void time_check_mul_overflow(size_t a, size_t b, const char* fn)
{
    if (a != 0 && a > SIZE_MAX / b)
        PANIC(fn);
}
#else
#    define time_check_mul_overflow(_a, _b, _fn)
#endif


static inline size_t time_ps_to_ns(size_t ps)
{
    return ps / 1000UL;
}

static inline size_t time_ns_to_ps(size_t ns)
{
    time_check_mul_overflow(ns, 1000UL, "time_ns_to_ps: conversion overflow");
    return ns * 1000UL;
}

static inline size_t time_ns_to_us(size_t ns)
{
    return ns / 1000UL;
}

static inline size_t time_us_to_ns(size_t us)
{
    time_check_mul_overflow(us, 1000UL, "time_us_to_ns: conversion overflow");
    return us * 1000UL;
}

static inline size_t time_us_to_ms(size_t us)
{
    return us / 1000UL;
}

static inline size_t time_ms_to_us(size_t ms)
{
    time_check_mul_overflow(ms, 1000UL, "time_ms_to_us: conversion overflow");
    return ms * 1000UL;
}

static inline size_t time_ms_to_s(size_t ms)
{
    return ms / 1000UL;
}

static inline size_t time_s_to_ms(size_t s)
{
    time_check_mul_overflow(s, 1000UL, "time_s_to_ms: conversion overflow");
    return s * 1000UL;
}

static inline size_t time_ps_to_us(size_t ps)
{
    return ps / 1000000UL;
}

static inline size_t time_us_to_ps(size_t us)
{
    time_check_mul_overflow(us, 1000000UL, "time_us_to_ps: conversion overflow");
    return us * 1000000UL;
}

static inline size_t time_ps_to_ms(size_t ps)
{
    return ps / 1000000000UL;
}

static inline size_t time_ms_to_ps(size_t ms)
{
    time_check_mul_overflow(
        ms,
        1000000000UL,
        "time_ms_to_ps: conversion overflow");
    return ms * 1000000000UL;
}

static inline size_t time_ns_to_ms(size_t ns)
{
    return ns / 1000000UL;
}

static inline size_t time_ms_to_ns(size_t ms)
{
    time_check_mul_overflow(ms, 1000000UL, "time_ms_to_ns: conversion overflow");
    return ms * 1000000UL;
}

static inline size_t time_ns_to_s(size_t ns)
{
    return ns / 1000000000UL;
}

static inline size_t time_s_to_ns(size_t s)
{
    time_check_mul_overflow(
        s,
        1000000000UL,
        "time_s_to_ns: conversion overflow");
    return s * 1000000000UL;
}
