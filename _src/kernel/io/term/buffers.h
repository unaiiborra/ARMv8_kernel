#pragma once

#include <kernel/io/term.h>
#include <lib/stdmacros.h>


typedef struct term_buffer {
    size_t buf_size;
    struct term_buffer* next;
    size_t head;
    size_t tail;
    uint8_t buf[];
} term_buffer;


static inline term_buffer_handle term_buffer_handle_new()
{
    return (term_buffer_handle) {
        .size = 0,
        .allocated_size = 0,
        .head_buf = NULL,
        .tail_buf = NULL,
    };
}


void term_buffer_push(term_buffer_handle* h, char c);
bool term_buffer_pop(term_buffer_handle* h, char* out);
bool term_buffer_peek(term_buffer_handle* h, char* out);
