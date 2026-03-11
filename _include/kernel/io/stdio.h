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
#define kprint(s) fkprint(IO_STDOUT, s)

#ifdef DEBUG
#    define dbg_printf(s, ...) fkprintf(IO_STDOUT, s, __VA_ARGS__)
#    define dbg_print(s) fkprint(IO_STDOUT, "\n\r[dbg] " s)
#else
#    define dbg_printf(s, ...)
#    define dbg_print(s)
#endif