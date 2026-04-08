#pragma once

#include <lib/stdmacros.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/align.h"
#include "lib/branch.h"
#include "lib/math.h"


typedef struct {
    size_t T_size_;
    size_t T_align_;

    size_t i_;

    size_t container_bytes_;
    void*  container_;
} kvec;

#define kvec(T) kvec

static inline kvec __kvec_new(kvec _)
{
    ASSERT(
        is_pow2(_.T_align_) && _.T_align_ <= PAGE_ALIGN &&
        _.T_align_ <= _.T_size_);
    ASSERT(is_aligned(_.T_size_, _.T_align_));

    return _;
}


#define kvec_new(T)                      \
    __kvec_new((kvec) {                  \
        .T_size_          = sizeof(T),   \
        .T_align_         = _Alignof(T), \
        .i_               = 0,           \
        .container_bytes_ = 0,           \
        .container_       = NULL,        \
    })


static inline void kvec_delete(kvec* k)
{
    if (k->container_)
        kfree(k->container_);

    *k = (kvec) {
        .T_size_          = 0,
        .T_align_         = 0,
        .i_               = 0,
        .container_bytes_ = 0,
        .container_       = NULL,
    };
}

static inline void kvec_empty(kvec* k)
{
    if (unlikely(k->i_ == 0))
        return;

    kfree(k->container_);

    k->i_               = 0;
    k->container_bytes_ = 0;
    k->container_       = NULL;
}


static inline size_t kvec_len(kvec k)
{
    return k.i_;
}


/// Pushes a new item to the end of the vector. It returns the item idx. It deep
/// copies the provided in
int64_t kvec_push(kvec* k, const void* in);

/// Removes the last item. It returns the idx of the removed element or -1 if
/// the vec is empty
int64_t kvec_pop(kvec* k, void* out);


bool kvec_set(const kvec* k, size_t i, const void* in, void* prev);
bool kvec_get_copy(const kvec* k, size_t i, void* out);
bool kvec_get_mut(const kvec* k, size_t i, void** out);


static inline void* kvec_data(const kvec* k)
{
    DEBUG_ASSERT((uintptr_t)k->container_ % k->T_align_ == 0);
    DEBUG_ASSERT((uintptr_t)k->container_ % k->T_size_ == 0);

    return k->container_;
}

#define kvec_dataT(T, k) (T*)kvec_data((k))
