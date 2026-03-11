#pragma once

#include <lib/stdmacros.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/align.h"
#include "lib/math.h"


typedef struct {
    size_t T_size_;
    size_t T_align_;

    size_t i_;

    size_t container_bytes_;
    void* container_;
} kvec;


static inline kvec __kvec_new(kvec _)
{
    ASSERT(
        is_pow2(_.T_align_) && _.T_align_ <= KPAGE_ALIGN &&
        _.T_align_ <= _.T_size_);
    ASSERT(is_aligned(_.T_size_, _.T_align_));

    return _;
}


#define kvec_T(T) kvec

#define kvec_new(T)              \
    __kvec_new((kvec) {          \
        .T_size_ = sizeof(T),    \
        .T_align_ = _Alignof(T), \
        .i_ = 0,                 \
        .container_bytes_ = 0,   \
        .container_ = NULL,      \
    })


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
