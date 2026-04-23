#pragma once

#include <lib/lock.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>


typedef uint64_t term_id;
typedef int32_t (*term_out)(const char c); // < 0: not taken, else taken


typedef struct {
    size_t              size;
    size_t              allocated_size;
    struct term_buffer* head_buf;
    struct term_buffer* tail_buf;
} term_buffer_handle;

typedef struct {
    term_id            id_;
    spinlock_t         lock_;
    term_out           out_;
    term_buffer_handle buf_;
} term_handle;


void term_new(term_handle* out, term_out output);
void term_delete(term_handle* h);

/*
 *  Prints
 *   @return: true if no notification of the term_out as ready is needed, else,
 *   the caller must notify when the provided output is ready to receive more
 *   data, in which case the term will fill the term_out function until a not
 *   taken (< 0) is received
 */
bool term_printc(term_handle* h, const char c);
bool term_prints(term_handle* h, const char* s);
bool term_printf(term_handle* h, const char* s, va_list ap);

/*
 *  notifies that the provided term_out is ready to receive more data.
 *  @return: true if no more notifications are needed. Else notifications when
 *  ready are still needed
 */
bool term_notify_ready(term_handle* h);




// void term_flush(term_handle* h);
