#pragma once

void io_early_init();
void io_init();


typedef enum {
    IO_STDOUT = 0,
    IO_STDWARN,
    IO_STDERR,
    IO_STDPANIC,
} io_out;


void io_flush(io_out io);

void fkprintf(io_out io, const char* s, ...);
void fkprint(io_out io, const char* s);


#define kprintf(s, ...) fkprintf(IO_STDOUT, s, __VA_ARGS__)
#define kprint(s)       fkprint(IO_STDOUT, s)


// // debug
// #define dbg_printf(lv, s, ...) fkprintf(IO_STDOUT, "\n\r(dbg) " s,
// __VA_ARGS__) #define dbg_print(s) fkprint(IO_STDOUT, "\n\r(dbg) " s

#define DEBUG_LOG   1
#define DEBUG_TRACE 2


#define dbg_print(lv, s)       __dbg_print_##lv(s)
#define dbg_printf(lv, s, ...) __dbg_printf_##lv(s, __VA_ARGS__)

#ifdef DEBUG
#    define __dbg_print_DEBUG_LOG(s) fkprint(IO_STDOUT, "\n\r(dbg) " s)
#    define __dbg_printf_DEBUG_LOG(s, ...) \
        fkprintf(IO_STDOUT, "\n\r(dbg) " s, __VA_ARGS__)
#    if DEBUG == DEBUG_LOG
#        define __dbg_print_DEBUG_TRACE(s)
#        define __dbg_printf_DEBUG_TRACE(s, ...)
#    elif DEBUG == DEBUG_TRACE
#        define __dbg_print_DEBUG_TRACE(s) fkprint(IO_STDOUT, "\n\r(dbg) " s)
#        define __dbg_printf_DEBUG_TRACE(s, ...) \
            fkprintf(IO_STDOUT, "\n\r(dbg) " s, __VA_ARGS__)
#    endif
#else
#    define __dbg_print_DEBUG_LOG(s)
#    define __dbg_printf_DEBUG_LOG(s, ...)
#    define __dbg_print_DEBUG_TRACE(s)
#    define __dbg_printf_DEBUG_TRACE(s, ...)
#endif