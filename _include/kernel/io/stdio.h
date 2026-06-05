#pragma once

#include <lib/ansi.h>

void io_init();

void printf(const char* s, ...);
void print(const char* s);

// // debug
// #define dbg_printf(lv, s, ...) printf(IO_STDOUT, DEBUG_TRACE_PREFIX s,
// __VA_ARGS__) #define dbg_print(s) print(IO_STDOUT, DEBUG_TRACE_PREFIX s

#define DEBUG_LOG   1
#define DEBUG_TRACE 2




#define dbg_print(lv, s)       __dbg_print_##lv(s)
#define dbg_printf(lv, s, ...) __dbg_printf_##lv(s, __VA_ARGS__)

#ifdef DEBUG
#    define DEBUG_ANSI_FG_COLOR             ANSI_FG_RGB(100, 100, 100)
#    define DEBUG_TRACE_PREFIX              DEBUG_ANSI_FG_COLOR "\n\r(dbg) "
#    define DEBUG_TRACE_ANSI_WRAP_STRING(s) DEBUG_TRACE_PREFIX s ANSI_RESET

#    define __dbg_print_DEBUG_LOG(s) \
        print(IO_STDOUT, DEBUG_TRACE_ANSI_WRAP_STRING(s))
#    define __dbg_printf_DEBUG_LOG(s, ...) printf(IO_STDOUT, s, __VA_ARGS__)
#    if DEBUG == DEBUG_LOG
#        define __dbg_print_DEBUG_TRACE(s)
#        define __dbg_printf_DEBUG_TRACE(s, ...)
#    elif DEBUG == DEBUG_TRACE
#        define __dbg_print_DEBUG_TRACE(s) \
            print(IO_STDOUT, DEBUG_TRACE_ANSI_WRAP_STRING(s))
#        define __dbg_printf_DEBUG_TRACE(s, ...) \
            printf(IO_STDOUT, DEBUG_TRACE_ANSI_WRAP_STRING(s), __VA_ARGS__)
#    endif
#else
#    define __dbg_print_DEBUG_LOG(s)
#    define __dbg_printf_DEBUG_LOG(s, ...)
#    define __dbg_print_DEBUG_TRACE(s)
#    define __dbg_printf_DEBUG_TRACE(s, ...)
#endif
