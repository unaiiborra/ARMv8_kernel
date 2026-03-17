#pragma once

#include <lib/lock.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    TERM_OUT_RES_OK = 0,
    TERM_OUT_RES_NOT_TAKEN = 1,
} term_out_result;


typedef uint64_t term_id;
typedef term_out_result (*term_out)(const char c);


typedef struct {
    size_t size;
    size_t allocated_size;
    struct term_buffer* head_buf;
    struct term_buffer* tail_buf;
} term_buffer_handle;

typedef struct {
    term_id id_;
    corelock_t lock_;
    term_out out_;
    term_buffer_handle buf_;
} term_handle;


void term_new(term_handle* out, term_out output);
void term_delete(term_handle* h);


/*
 *  Prints
 */
void term_printc(term_handle* h, const char c);
void term_prints(term_handle* h, const char* s);
void term_printf(term_handle* h, const char* s, va_list ap);


void term_flush(term_handle* h);
