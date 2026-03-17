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


static inline void alloc_tail(term_buffer_handle* h, size_t size)
{
    DEBUG_ASSERT(size >= KPAGE_SIZE);
    DEBUG_ASSERT(size % KPAGE_SIZE == 0);

    term_buffer* new_tail = kmalloc(size);

    *new_tail = (term_buffer) {
        .buf_size = size - sizeof(term_buffer),
        .next = NULL,
        .head = 0,
        .tail = 0,
    };

    if (!h->tail_buf) {
        h->head_buf = new_tail;
        h->tail_buf = new_tail;
    }
    else {
        h->tail_buf->next = new_tail;
        h->tail_buf = new_tail;
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


void term_buffer_push(term_buffer_handle* h, char c)
{
    DEBUG_ASSERT(h);


    if (!h->tail_buf || h->tail_buf->tail >= h->tail_buf->buf_size) {
        size_t buf_size = max(KPAGE_SIZE, h->allocated_size);

        alloc_tail(h, buf_size);
    }


    term_buffer* tail_buf = h->tail_buf;

    tail_buf->buf[tail_buf->tail++] = c;

    h->size++;
}


bool term_buffer_pop(term_buffer_handle* h, char* out)
{
    DEBUG_ASSERT(h && out);

    if (h->size == 0) {
        DEBUG_ASSERT(!h->head_buf && !h->tail_buf);
        return false;
    }

    term_buffer* head_buf = h->head_buf;

    DEBUG_ASSERT(head_buf->head < head_buf->buf_size);

    *out = head_buf->buf[head_buf->head++];
    h->size--;


    if (head_buf->head == head_buf->tail)
        free_head(h, head_buf);


    return true;
}


bool term_buffer_peek(term_buffer_handle* h, char* out)
{
    if (h->size == 0) {
        DEBUG_ASSERT(!h->head_buf && !h->tail_buf);
        return false;
    }

    term_buffer* head_buf = h->head_buf;

    DEBUG_ASSERT(head_buf->head < head_buf->buf_size);

    *out = head_buf->buf[head_buf->head];

    return true;
}
