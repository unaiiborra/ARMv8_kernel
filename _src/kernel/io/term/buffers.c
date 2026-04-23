#include "buffers.h"

#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/align.h>
#include <lib/lock.h>
#include <lib/math.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stdbool.h>
#include <stddef.h>

#include "lib/branch.h"


static inline void alloc_tail(term_buffer_handle* h, size_t size)
{
    DEBUG_ASSERT(size >= PAGE_SIZE);
    DEBUG_ASSERT(size % PAGE_SIZE == 0);

    term_buffer* new_tail = kmalloc(size);

    *new_tail = (term_buffer) {
        .buf_size = size - sizeof(term_buffer),
        .next     = NULL,
        .head     = 0,
        .tail     = 0,
    };

    if (!h->tail_buf) {
        h->head_buf = new_tail;
        h->tail_buf = new_tail;
    }
    else {
        h->tail_buf->next = new_tail;
        h->tail_buf       = new_tail;
    }

    h->allocated_size += size;
}


static inline void free_head(term_buffer_handle* h, term_buffer* head)
{
    DEBUG_ASSERT(head);

    h->allocated_size -= head->buf_size + sizeof(term_buffer);
    h->head_buf = h->head_buf->next;

    if (h->head_buf == NULL) {
        h->tail_buf = NULL;

        DEBUG_ASSERT(h->allocated_size == 0 && h->size == 0);
    }

    raw_kfree(head);
}


size_t term_buffer_push(term_buffer_handle* h, char c)
{
    DEBUG_ASSERT(h);


    if (!h->tail_buf || h->tail_buf->tail >= h->tail_buf->buf_size) {
        size_t buf_size = max(PAGE_SIZE, h->allocated_size);

        alloc_tail(h, buf_size);
    }


    term_buffer* tail_buf = h->tail_buf;

    tail_buf->buf[tail_buf->tail++] = c;

    return ++h->size;
}


size_t term_buffer_peek(term_buffer_handle* h, char* out)
{
    if (unlikely(h->size == 0)) {
        DEBUG_ASSERT(!h->head_buf && !h->tail_buf);
        return 0;
    }

    term_buffer* head_buf = h->head_buf;

    DEBUG_ASSERT(head_buf->head < head_buf->buf_size);

    *out = head_buf->buf[head_buf->head];

    return h->size;
}


size_t term_buffer_remove_from_head(term_buffer_handle* h)
{
    DEBUG_ASSERT(h);

    if (unlikely(h->size == 0)) {
        DEBUG_ASSERT(!h->head_buf && !h->tail_buf);
        return 0;
    }

    term_buffer* head_buf = h->head_buf;

    h->size--;
    head_buf->head++;

    if (head_buf->head == head_buf->tail)
        free_head(h, head_buf);

    return h->size;
}