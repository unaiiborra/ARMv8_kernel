#pragma once

#include <arm/exceptions/exceptions.h>
#include <lib/branch.h>
#include <stddef.h>
#include <stdint.h>
typedef enum {
    PANIC_REASON_UNDEFINED = 0,
    PANIC_REASON_EXCEPTION = 1,
    PANIC_REASON_MANUAL_ABORT = 2, // does not come from an exception
} panic_reason;


/*
 *  Manual aborts
 */
typedef enum {
    PANIC_LANG_UNDEFINED = 0,
    PANIC_LANG_ASM = 1,
    PANIC_LANG_C = 2,
    PANIC_LANG_RUST = 3,
} panic_lang;

typedef struct {
    const char* file;
    int32_t line; // -1 = unknown
    int32_t col;  // -1 = unknown
} panic_location;


/*
 *  Exception panics
 */
typedef enum {
    PANIC_EXCEPTION_CUR_SP0 = 0,
    PANIC_EXCEPTION_CUR_SPX = 1,
    PANIC_EXCEPTION_LOW_A32 = 2,
    PANIC_EXCEPTION_LOW_A64 = 3,
} panic_exception_src;

typedef enum {
    PANIC_EXCEPTION_TYPE_SYNC = 0,
    PANIC_EXCEPTION_TYPE_IRQ = 1,
    PANIC_EXCEPTION_TYPE_FIQ = 2,
    PANIC_EXCEPTION_TYPE_SERROR = 3,
} panic_exception_type;

typedef struct {
    panic_reason reason;
    const char* message;
    arm_exception_status exception_status;
    union {
        struct {
            panic_lang lang;
            panic_location location;
        } manual_abort;

        struct {
            panic_exception_src src;
            panic_exception_type type;
        } exception;
    } info;
} panic_info;


/// non recoverable panic
_Noreturn __attribute__((cold)) void panic(panic_info panic_info);

/// recoverable panic
void __attribute__((cold)) panicr(panic_info panic_info);


/*
 *  macros
 */
#define PANIC0() PANIC1("")
#define PANIC1(msg)                                                      \
    panic((panic_info) {.reason = PANIC_REASON_MANUAL_ABORT,             \
                        .message = msg,                                  \
                        .exception_status = arm_exceptions_get_status(), \
                        .info = {                                        \
                            .manual_abort =                              \
                                {                                        \
                                    .lang = PANIC_LANG_C,                \
                                    .location =                          \
                                        {                                \
                                            .file = __FILE__,            \
                                            .line = __LINE__,            \
                                            .col = -1,                   \
                                        },                               \
                                },                                       \
                        }})


#define PANIC_SELECT(_0, _1, NAME, ...) NAME
#define PANIC(...) PANIC_SELECT(_, ##__VA_ARGS__, PANIC1, PANIC0)(__VA_ARGS__)


#define PANICR0() PANICR("")
#define PANICR1(msg)                                                      \
    panicr((panic_info) {.reason = PANIC_REASON_MANUAL_ABORT,             \
                         .message = msg,                                  \
                         .exception_status = arm_exceptions_get_status(), \
                         .info = {                                        \
                             .manual_abort =                              \
                                 {                                        \
                                     .lang = PANIC_LANG_C,                \
                                     .location =                          \
                                         {                                \
                                             .file = __FILE__,            \
                                             .line = __LINE__,            \
                                             .col = -1,                   \
                                         },                               \
                                 },                                       \
                         }})

#define PANICR_SELECT(_0, _1, NAME, ...) NAME
#define PANICR(...) \
    PANICR_SELECT(_, ##__VA_ARGS__, PANICR1, PANICR0)(__VA_ARGS__)


#define _ASSERT_1(cond) \
    (LIKELY(cond) ? (void)0 : PANIC("assertion failed: " #cond))
#define _ASSERT_2(cond, msg) \
    (LIKELY(cond) ? (void)0 : PANIC("assertion failed: " #cond " -> " msg))

#define _ASSERT_SELECT(_0, _1, _2, NAME, ...) NAME
#define ASSERT(...) \
    _ASSERT_SELECT(_, ##__VA_ARGS__, _ASSERT_2, _ASSERT_1)(__VA_ARGS__)

#ifdef DEBUG
#    define _DEBUG_ASSERT_1(cond) \
        (LIKELY(cond) ? (void)0 : PANIC("debug assertion failed: " #cond))
#    define _DEBUG_ASSERT_2(cond, msg) \
        (LIKELY(cond) ? (void)0        \
                      : PANIC("debug assertion failed: " #cond " -> " msg))
#    define DEBUG_ASSERT(...)                                               \
        _ASSERT_SELECT(_, ##__VA_ARGS__, _DEBUG_ASSERT_2, _DEBUG_ASSERT_1)( \
            __VA_ARGS__)
#else
#    define DEBUG_ASSERT(...)
#endif
