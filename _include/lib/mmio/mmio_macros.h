#pragma once

#include <lib/mmio/mmio_regs.h>
#include <stddef.h>
#include <stdint.h>
#define MMIO_DECLARE_REG32_VALUE_STRUCT(RegValueStructName) \
    typedef struct {                                        \
        uint32_t val;                                       \
    } RegValueStructName;

#define MMIO_DECLARE_REG64_VALUE_STRUCT(RegValueStructName) \
    typedef struct {                                        \
        uint64_t val;                                       \
    } RegValueStructName;

// Register getters

#define MMIO_DECLARE_REG32_READER(                                        \
    periph_name,                                                          \
    reg_name,                                                             \
    RegValueStruct,                                                       \
    offset)                                                               \
    static inline RegValueStruct periph_name##_##reg_name##_read(         \
        uintptr_t base)                                                   \
    {                                                                     \
        return (RegValueStruct) {.val = *((reg32_ptr)(base + (offset)))}; \
    }

#define MMIO_DECLARE_REG32_READER_N_OFFSET(                            \
    periph_name,                                                       \
    reg_name,                                                          \
    RegValueStruct,                                                    \
    offset_macro)                                                      \
    static inline RegValueStruct periph_name##_##reg_name##_read(      \
        uintptr_t base,                                                \
        size_t    n)                                                   \
    {                                                                  \
        return (RegValueStruct) {                                      \
            .val = *((reg32_ptr)(base + (uintptr_t)offset_macro(n)))}; \
    }

#define MMIO_DECLARE_REG64_READER(                                        \
    periph_name,                                                          \
    reg_name,                                                             \
    RegValueStruct,                                                       \
    offset)                                                               \
    static inline RegValueStruct periph_name##_##reg_name##_read(         \
        uintptr_t base)                                                   \
    {                                                                     \
        return (RegValueStruct) {.val = *((reg64_ptr)(base + (offset)))}; \
    }

#define MMIO_DECLARE_REG64_READER_N_OFFSET(                            \
    periph_name,                                                       \
    reg_name,                                                          \
    RegValueStruct,                                                    \
    offset_macro)                                                      \
    static inline RegValueStruct periph_name##_##reg_name##_read(      \
        uintptr_t base,                                                \
        size_t    n)                                                   \
    {                                                                  \
        return (RegValueStruct) {                                      \
            .val = *((reg64_ptr)(base + (uintptr_t)offset_macro(n)))}; \
    }

// Register setters

#define MMIO_DECLARE_REG32_WRITER(                       \
    periph_name,                                         \
    reg_name,                                            \
    RegValueStruct,                                      \
    offset)                                              \
    static inline void periph_name##_##reg_name##_write( \
        uintptr_t      base,                             \
        RegValueStruct v)                                \
    {                                                    \
        *((reg32_ptr)(base + (offset))) = v.val;         \
    }

#define MMIO_DECLARE_REG32_WRITER_N_OFFSET(                        \
    periph_name,                                                   \
    reg_name,                                                      \
    RegValueStruct,                                                \
    offset_macro)                                                  \
    static inline void periph_name##_##reg_name##_write(           \
        uintptr_t      base,                                       \
        size_t         n,                                          \
        RegValueStruct v)                                          \
    {                                                              \
        *((reg32_ptr)(base + (uintptr_t)offset_macro(n))) = v.val; \
    }

#define MMIO_DECLARE_REG64_WRITER(                       \
    periph_name,                                         \
    reg_name,                                            \
    RegValueStruct,                                      \
    offset)                                              \
    static inline void periph_name##_##reg_name##_write( \
        uintptr_t      base,                             \
        RegValueStruct v)                                \
    {                                                    \
        *((reg64_ptr)(base + (offset))) = v.val;         \
    }

#define MMIO_DECLARE_REG64_WRITER_N_OFFSET(                        \
    periph_name,                                                   \
    reg_name,                                                      \
    RegValueStruct,                                                \
    offset_macro)                                                  \
    static inline void periph_name##_##reg_name##_write(           \
        uintptr_t      base,                                       \
        size_t         n,                                          \
        RegValueStruct v)                                          \
    {                                                              \
        *((reg64_ptr)(base + (uintptr_t)offset_macro(n))) = v.val; \
    }

/*
 *      Bit field fns
 */

#define MMIO_DECLARE_BIT_FIELD_GETTER(                                         \
    periph_name,                                                               \
    reg_name,                                                                  \
    bf_name,                                                                   \
    RegValueStruct,                                                            \
    T,                                                                         \
    SHIFT,                                                                     \
    MASK)                                                                      \
    static inline T periph_name##_##reg_name##_##bf_name##_get(                \
        const RegValueStruct r)                                                \
    {                                                                          \
        typedef typeof(r.val) reg_t;                                           \
        _Static_assert(                                                        \
            (MASK >> SHIFT) << SHIFT == MASK,                                  \
            "MASK/SHIFT mismatch");                                            \
        _Static_assert(__builtin_constant_p(MASK), "MASK must be constant");   \
        _Static_assert(__builtin_constant_p(SHIFT), "SHIFT must be constant"); \
                                                                               \
        return (T)((r.val & (reg_t)(MASK)) >> (reg_t)(SHIFT));                 \
    }

#define MMIO_DECLARE_BIT_FIELD_SETTER(                                         \
    periph_name,                                                               \
    reg_name,                                                                  \
    bf_name,                                                                   \
    RegValueStruct,                                                            \
    T,                                                                         \
    SHIFT,                                                                     \
    MASK)                                                                      \
    static inline void periph_name##_##reg_name##_##bf_name##_set(             \
        RegValueStruct* r,                                                     \
        T               v)                                                     \
    {                                                                          \
        typedef typeof(r->val) reg_t;                                          \
        _Static_assert(                                                        \
            (MASK >> SHIFT) << SHIFT == MASK,                                  \
            "MASK/SHIFT mismatch");                                            \
        _Static_assert(__builtin_constant_p(MASK), "MASK must be constant");   \
        _Static_assert(__builtin_constant_p(SHIFT), "SHIFT must be constant"); \
                                                                               \
        r->val = (r->val & ~((reg_t)(MASK))) |                                 \
                 (((reg_t)(v) << (reg_t)(SHIFT)) & (reg_t)(MASK));             \
    }
