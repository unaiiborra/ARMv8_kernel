#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>

#define UART_DECLARE_BIT_FIELD_GETTER( \
    reg_name,                          \
    bf_name,                           \
    RegValueStruct,                    \
    T,                                 \
    SHIFT,                             \
    MASK)                              \
    MMIO_DECLARE_BIT_FIELD_GETTER(     \
        UART,                          \
        reg_name,                      \
        bf_name,                       \
        RegValueStruct,                \
        T,                             \
        SHIFT,                         \
        MASK)

#define UART_DECLARE_BIT_FIELD_SETTER( \
    reg_name,                          \
    bf_name,                           \
    RegValueStruct,                    \
    T,                                 \
    SHIFT,                             \
    MASK)                              \
    MMIO_DECLARE_BIT_FIELD_SETTER(     \
        UART,                          \
        reg_name,                      \
        bf_name,                       \
        RegValueStruct,                \
        T,                             \
        SHIFT,                         \
        MASK)
