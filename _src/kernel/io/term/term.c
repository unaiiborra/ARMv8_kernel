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

void term_new(term_handle* out, term_out output)
{
    *out = (term_handle) {
        .id_   = atomic_fetch_add(&uniqueid_count, 1),
        .lock_ = SPINLOCK_INIT,
        .buf_  = term_buffer_handle_new(),
        .out_  = output,
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
    irq_spinlocked(&h->lock_)
    {
        if (h->buf_.size == 0) {
            int32_t result = h->out_(c);

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

    irqlock_t f = spin_lock_irqsave(&h->lock_);

    if (h->buf_.size == 0) {
        while (*s && (h->out_(*s) >= 0))
            s++;

        finished_output = *s == '\0';
    }
    else
        finished_output = false;

    while (*s)
        term_buffer_push(&h->buf_, *s++);

    spin_unlock_irqrestore(&h->lock_, f);

    return finished_output;
}


static void putfmt(char c, void* args)
{
    kvec(char)* string = args;

    kvec_push(string, &c);
}


bool term_printf(term_handle* h, const char* s, va_list ap)
{
    // format the string
    defer(kvec_delete) kvec(char) string = kvec_new(char);
    str_fmt_print(putfmt, &string, s, ap);

    return term_prints(h, kvec_data(&string));
}


bool term_notify_ready(term_handle* h)
{
    irqlock_t f = spin_lock_irqsave(&h->lock_);

    if (unlikely(h->buf_.size == 0)) {
        spin_unlock_irqrestore(&h->lock_, f);
        return true;
    }

    bool finished_output;

    loop
    {
        char c;

        term_buffer_peek(&h->buf_, &c);

        if (likely(h->out_(c) >= 0)) {
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

    spin_unlock_irqrestore(&h->lock_, f);

    return finished_output;
}
