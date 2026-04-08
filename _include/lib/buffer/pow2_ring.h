#pragma once
#include <stdbool.h>
#include <stddef.h>


typedef struct pow2_ring_buffer {
    void*  buf;    // backing buffer
    size_t size;   // number of elements (power of 2)
    size_t t_size; // size of each element in bytes
    size_t tail;
    size_t head;
    bool   overwrite;
} pow2_ring_buffer;

pow2_ring_buffer
pow2_ring_buffer_new(void* buf, size_t size, size_t t_size, bool overwrite);

/* Push element (copies t_size bytes)
 * Returns false if buffer was full
 */
bool pow2_ring_buffer_push(pow2_ring_buffer* rb, const void* v);

/* Pop element (copies into out)
 * Returns false if empty
 */
bool pow2_ring_buffer_pop(pow2_ring_buffer* rb, void* out);
