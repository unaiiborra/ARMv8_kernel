#include "kernel/io/term.h"

#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <lib/branch.h>
#include <lib/lock.h>
#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "buffers.h"


static uint64_t uniqueid_count = 0;

void term_new(term_handle* out, term_out output, void* ctx)
{
    *out = (term_handle) {
        .id_   = atomic_fetch_add(&uniqueid_count, 1),
        .lock_ = SPINLOCK_INIT,
        .buf_  = term_buffer_handle_new(),
        .out_  = output,
        .ctx   = ctx,
    };
}


void term_delete(term_handle* h)
{
    term_buffer* cur = h->buf_.head_buf;

    while (cur) {
        term_buffer* next = cur->next;

        kfree(cur);

        cur = next;
    }

    *h = (term_handle) {0};
}


bool term_printc(term_handle* h, const char c)
{
    spinlocked_irqsave(&h->lock_)
    {
        if (h->buf_.size == 0) {
            int32_t result = h->out_(c, h->ctx);

            if (result >= 0)
                return true;
        }

        term_buffer_push(&h->buf_, c);
    }

    return false;
}


bool term_prints(term_handle* h, const char* s)
{
    bool finished_output;

    irqflags_t f = spinlock_acquire_irqsave(&h->lock_);

    if (h->buf_.size == 0) {
        while (*s && (h->out_(*s, h->ctx) >= 0))
            s++;

        finished_output = *s == '\0';
    }
    else
        finished_output = false;

    while (*s)
        term_buffer_push(&h->buf_, *s++);

    spinlock_release_irqrestore(&h->lock_, f);

    return finished_output;
}

bool term_print_slice(term_handle* h, const char* s, size_t len)
{
    size_t i = 0;

    irqflags_t f = spinlock_acquire_irqsave(&h->lock_);

    if (h->buf_.size == 0)
        for (; i < len; i++) {
            char c = s[i];

            if (h->out_(c, h->ctx) < 0)
                break;
        }

    bool finished = !(i < len);

    for (; i < len; i++) {
        char c = s[i];
        term_buffer_push(&h->buf_, c);
    }

    spinlock_release_irqrestore(&h->lock_, f);

    return finished;
}

size_t term_remove_head(term_handle* h, char* buf, size_t count)
{
    size_t popped = 0;

    irqflags_t f = spinlock_acquire_irqsave(&h->lock_);

    popped = term_buffer_pop_n(&h->buf_, count, buf);

    spinlock_release_irqrestore(&h->lock_, f);

    return popped;
}

static void putfmt(char c, void* args)
{
    kvec(char)* string = args;

    kvec_push(string, &c);
}


bool term_outf(term_handle* h, const char* s, va_list ap)
{
    // format the string
    defer(kvec_delete) kvec(char) string = kvec_new(char);
    str_fmt_print(putfmt, &string, s, ap);

    return term_prints(h, kvec_data(&string));
}


static bool term_notify_ready_unlocked(term_handle* h)
{
    if (unlikely(h->buf_.size == 0)) {
        return true;
    }

    bool finished_output;

    loop
    {
        char c;

        term_buffer_peek(&h->buf_, &c);

        if (likely(h->out_(c, h->ctx) >= 0)) {
            // taken, apply the peek as a pop
            size_t size = term_buffer_remove_from_head(&h->buf_);

            if (unlikely(size == 0)) {
                finished_output = true;
                break;
            }
        }
        else {
            // not taken
            finished_output = false;
            break;
        }
    }

    return finished_output;
}


bool term_notify_ready(term_handle* h)
{
    irqflags_t irqflags = spinlock_acquire_irqsave(&h->lock_);

    bool finished_output = term_notify_ready_unlocked(h);

    spinlock_release_irqrestore(&h->lock_, irqflags);

    return finished_output;
}



void term_flush(term_handle* h)
{
    irqflags_t irqflags = spinlock_acquire_irqsave(&h->lock_);

    while (!term_notify_ready_unlocked(h)) {
        asm volatile("nop");
    }

    spinlock_release_irqrestore(&h->lock_, irqflags);
}
