#include "kernel/io/term.h"

#include <lib/branch.h>
#include <lib/lock/irqlock.h>
#include <lib/lock/spinlock_irq.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>

#include "buffers.h"
#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/lock/corelock.h"


static uint64_t uniqueid_count;

void term_new(term_handle* out, term_out output)
{
    *out = (term_handle) {
        .id_ = __atomic_add_fetch(&uniqueid_count, 1, __ATOMIC_SEQ_CST),
        .lock_ = {0},
        .buf_ = term_buffer_handle_new(),
        .out_ = output,
    };

    corelock_init(&out->lock_);
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


/*
 *  It attempts to push the character to the out fn, if it is not taken, saves
 * it in a buffer. If later calls are made to this fn, if the term buffer is not
 * empty, it will push directly into the buffer in order to mantain coherency of
 * the lifo, (even if the out fn could respond with an OK).
 */
static inline void putc(term_handle* h, char c)
{
    if (h->buf_.size == 0) {
        term_out_result result = h->out_(c);

        if (result == TERM_OUT_RES_OK)
            return;
    }

    term_buffer_push(&h->buf_, c);
}


void term_printc(term_handle* h, const char c)
{
    irqlock_t f = irq_lock();

    core_lock(&h->lock_);

    putc(h, c);

    core_unlock(&h->lock_);
    irq_unlock(f);
}

void term_prints(term_handle* h, const char* s)
{
    irqlock_t f = irq_lock();

    core_lock(&h->lock_);

    while (*s)
        putc(h, *s++);

    core_unlock(&h->lock_);
    irq_unlock(f);
}


static void putfmt(char c, void* args)
{
    term_handle* h = args;

    putc(h, c);
}


/// The formatting happens before the printing because in future versions
/// checking the result of the term_out will be needed and that cannot happen
/// inside the fmt fn. This fn will need a big remake when kmalloc is
/// implemented TODO:
void term_printf(term_handle* h, const char* s, va_list ap)
{
    irqlock_t f = irq_lock();

    core_lock(&h->lock_);

    str_fmt_print(putfmt, h, s, ap);

    core_unlock(&h->lock_);
    irq_unlock(f);
}


void term_flush(term_handle* h)
{
    DEBUG_ASSERT(h);

    if (h->buf_.size == 0)
        return;


    __attribute((unused)) char peek, pop;


    irqlock_t f = irq_lock();
    core_lock(&h->lock_);

    loop
    {
        if (!term_buffer_peek(&h->buf_, &peek))
            break;

        term_out_result res = h->out_(peek);

        if (res == TERM_OUT_RES_NOT_TAKEN)
            break;

        __attribute((unused)) bool pop_res = term_buffer_pop(&h->buf_, &pop);

        DEBUG_ASSERT(pop_res && peek == pop);
    }

    core_unlock(&h->lock_);
    irq_unlock(f);
}
