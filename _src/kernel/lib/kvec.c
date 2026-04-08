#include <kernel/io/stdio.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdmacros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MIN_VEC_ITEMS 8
#define MIN_ORDER     6


static inline size_t pow2_bytes_for(size_t size, size_t amount)
{
    size_t bytes = size * amount;

    DEBUG_ASSERT(amount == 0 || bytes / amount == size);

    size_t o = MIN_ORDER; // min order 6, 64 bytes

    while (bytes > power_of2(o))
        o++;

    return power_of2(o);
}


static void upsize_vec(kvec* k)
{
    bool remalloc = k->container_ != NULL;

#ifdef DEBUG
    if (!remalloc)
        DEBUG_ASSERT(
            k->container_bytes_ == 0 && k->i_ == 0 && k->container_ == NULL);
    else
        DEBUG_ASSERT(is_pow2(k->container_bytes_));
#endif

    size_t bytes = remalloc ? (k->container_bytes_ * 2)
                            : pow2_bytes_for(k->T_size_, MIN_VEC_ITEMS);


    void* new_container = kmalloc(bytes);

    if (remalloc) {
        memcpy(new_container, k->container_, k->i_ * k->T_size_);
        kfree(k->container_);
    }

    k->container_       = new_container;
    k->container_bytes_ = bytes;
}


static void downsize_vec(kvec* k)
{
    size_t bytes = k->container_bytes_ / 2;

    DEBUG_ASSERT(k->container_ != NULL);
    ASSERT(is_pow2(k->container_bytes_));
    ASSERT(k->container_);
    ASSERT(k->i_ * k->T_size_ <= bytes);


    if (k->i_ == 0) {
        kfree(k->container_);
        k->container_       = NULL;
        k->container_bytes_ = 0;

        return;
    }


    size_t min_bytes = pow2_bytes_for(k->T_size_, MIN_VEC_ITEMS);
    if (bytes < min_bytes)
        return;


    void* new_container = kmalloc(bytes);

    memcpy(new_container, k->container_, k->i_ * k->T_size_);
    kfree(k->container_);

    k->container_       = new_container;
    k->container_bytes_ = bytes;
}




int64_t kvec_push(kvec* k, const void* in)
{
    uintptr_t i_offset = k->i_ * k->T_size_;

    if (i_offset + k->T_size_ > k->container_bytes_)
        upsize_vec(k);


    void* dst = (void*)((uintptr_t)k->container_ + i_offset);
    memcpy(dst, in, k->T_size_);


    return k->i_++;
}


int64_t kvec_pop(kvec* k, void* out)
{
    if (k->i_ == 0)
        return -1;

    k->i_--;

    uintptr_t i_offset = k->i_ * k->T_size_;
    void*     dst      = (void*)((uintptr_t)k->container_ + i_offset);

    if (out)
        memcpy(out, dst, k->T_size_);

    size_t capacity = k->container_bytes_ / k->T_size_;

    if (k->i_ <= capacity / 4)
        downsize_vec(k);


    return k->i_;
}


bool kvec_set(const kvec* k, size_t i, const void* in, void* prev)
{
    if (!in || i >= k->i_)
        return false;

    if (prev) {
        void* src = (void*)((uintptr_t)k->container_ + i * k->T_size_);
        memcpy(prev, src, k->T_size_);
    }


    void* dst = (void*)((uintptr_t)k->container_ + i * k->T_size_);
    memcpy(dst, in, k->T_size_);

    return true;
}


bool kvec_get_copy(const kvec* k, size_t i, void* out)
{
    DEBUG_ASSERT(k->T_size_ % k->T_align_ == 0);

    if (i >= k->i_ || !out)
        return false;

    void* src = (void*)((uintptr_t)k->container_ + i * k->T_size_);

    switch (k->T_size_) {
        case sizeof(uint8_t):
            *(uint8_t*)out = *(uint8_t*)src;
            break;

        case sizeof(uint16_t):
            *(uint16_t*)out = *(uint16_t*)src;
            break;

        case sizeof(uint32_t):
            *(uint32_t*)out = *(uint32_t*)src;
            break;

        case sizeof(uint64_t):
            *(uint64_t*)out = *(uint64_t*)src;
            break;

        default:
            memcpy(out, src, k->T_size_);
    }

    return true;
}


bool kvec_get_mut(const kvec* k, size_t i, void** out)
{
    if (i >= k->i_ || !out)
        return false;

    void* src = (void*)((uintptr_t)k->container_ + i * k->T_size_);

    *out = src;

    return true;
}
