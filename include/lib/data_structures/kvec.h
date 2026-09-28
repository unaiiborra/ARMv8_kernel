#pragma once

#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/align.h"
#include "lib/branch.h"
#include "lib/math.h"

typedef struct kvec {
	size_t T_size_;
	size_t T_align_;

	size_t i_;

	size_t container_bytes_;
	void *container_;
} kvec_t;

#define kvec(T)        kvec_t
#define scoped_kvec(T) deferT(kvec(T), kvec_delete)

#define KVEC_INIT(T)                                                                               \
	(kvec_t)                                                                                   \
	{                                                                                          \
		.T_size_ = sizeof(T) + __KVEC_CHECK_T(T), .T_align_ = _Alignof(T), .i_ = 0,        \
		.container_bytes_ = 0, .container_ = NULL,                                         \
	}

static inline void kvec_delete(kvec_t *k)
{
	if (k->container_) {
		kfree(k->container_);
	}

	*k = (kvec_t){
		.T_size_ = 0,
		.T_align_ = 0,
		.i_ = 0,
		.container_bytes_ = 0,
		.container_ = NULL,
	};
}

static inline void kvec_empty(kvec_t *k)
{
	if (unlikely(k->i_ == 0)) {
		return;
	}

	kfree(k->container_);

	k->i_ = 0;
	k->container_bytes_ = 0;
	k->container_ = NULL;
}

[[gnu::always_inline]] static inline size_t kvec_len(const kvec_t *k)
{
	return k->i_;
}

/// Pushes a new item to the end of the vector. It returns the item idx. It
/// copies the provided in
size_t kvec_push(kvec_t *k, const void *in);

/// Removes the last item. It returns the idx of the removed element or -1 if
/// the vec is empty
int64_t kvec_pop(kvec_t *k, void *out);

bool kvec_set(const kvec_t *k, size_t i, const void *in, void *prev);
bool kvec_get_copy(const kvec_t *k, size_t i, void *out);
bool kvec_get_mut(const kvec_t *k, size_t i, void **out);

[[gnu::always_inline]] static inline void *kvec_data(const kvec_t *k)
{
	DEBUG_ASSERT((uintptr_t)k->container_ % k->T_align_ == 0);

	return k->container_;
}

#define kvec_dataT(T, k) (T *)kvec_data((k))

/* Private macro helpers */
#define __STATIC_ASSERT_EXPR(cond, msg)                                                            \
	(0 * sizeof(struct {                                                                       \
		 _Static_assert(cond, msg);                                                        \
		 char dummy_;                                                                      \
	 }))

#define __KVEC_CHECK_T(T)                                                                          \
	(__STATIC_ASSERT_EXPR(IS_POW2(_Alignof(T)), "kvec: alignment not a power of 2") +          \
	 __STATIC_ASSERT_EXPR(_Alignof(T) <= PAGE_ALIGN, "kvec: alignment > PAGE_ALIGN") +         \
	 __STATIC_ASSERT_EXPR(_Alignof(T) <= sizeof(T), "kvec: alignment > size") +                \
	 __STATIC_ASSERT_EXPR(IS_ALIGNED(sizeof(T), _Alignof(T)), "kvec: size not aligned"))
