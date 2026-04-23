#pragma once

#include <kernel/io/term.h>
#include <lib/stdmacros.h>


typedef struct term_buffer {
    size_t              buf_size;
    struct term_buffer* next;
    size_t              head;
    size_t              tail;
    uint8_t             buf[];
} term_buffer;


static inline term_buffer_handle term_buffer_handle_new()
{
    return (term_buffer_handle) {
        .size           = 0,
        .allocated_size = 0,
        .head_buf       = NULL,
        .tail_buf       = NULL,
    };
}


size_t term_buffer_push(term_buffer_handle* h, char c);

size_t term_buffer_peek(term_buffer_handle* h, char* out);
/// like pop but does not return the char. Used to make a peek work as it if was
/// a pop if needed. Returns the size after the last remove
size_t term_buffer_remove_from_head(term_buffer_handle* h);