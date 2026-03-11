#pragma once

#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <lib/stdmacros.h>
#include <stddef.h>
#include <stdint.h>

#include "../vmalloc.h"

typedef bitfield8 bf;

#define DATA_BYTES (KPAGE_SIZE - (2 * sizeof(union vmalloc_container*)))
#define DATA_ALIGN (_Alignof(uint64_t))

// free va node
typedef struct fva_node {
    struct fva_node* next;

    v_uintptr_t start;
    size_t size;
} fva_node;

// reserved va node
typedef struct rva_node {
    struct rva_node* next;

    v_uintptr_t start;
    size_t size;
    vmalloc_mdt mdt;
} rva_node;


#define FVA_NODE_COUNT (DATA_BYTES / sizeof(fva_node) - 1)
#define RVA_NODE_COUNT (DATA_BYTES / sizeof(rva_node) - 1)

typedef struct {
    // bitfield that represents if a node is reserved (1) or free for
    // reservation (0).
    _Alignas(DATA_ALIGN) bf
        reserved_nodes[BITFIELD_COUNT_FOR(FVA_NODE_COUNT, bf)];

    fva_node nodes[FVA_NODE_COUNT];
} fva_container_data;

typedef struct {
    // bitfield that represents if a node is reserved (1) or free for
    // reservation (0).
    _Alignas(DATA_ALIGN) bf
        reserved_nodes[BITFIELD_COUNT_FOR(RVA_NODE_COUNT, bf)];

    rva_node nodes[RVA_NODE_COUNT];
} rva_container_data;


union vmalloc_container;

typedef struct {
    union vmalloc_container* next;
    union vmalloc_container* prev;
} vmalloc_container_hdr;

typedef union vmalloc_container {
    struct {
        _Alignas(KPAGE_ALIGN) vmalloc_container_hdr hdr;
        uint64_t data[DATA_BYTES / sizeof(uint64_t)];
    } undef;

    struct {
        _Alignas(KPAGE_ALIGN) vmalloc_container_hdr hdr;
        fva_container_data data;
    } fva;

    struct {
        _Alignas(KPAGE_ALIGN) vmalloc_container_hdr hdr;
        rva_container_data data;
    } rva;
} vmalloc_container;


_Static_assert(
    sizeof(fva_container_data) <= DATA_BYTES &&
    _Alignof(fva_container_data) == DATA_ALIGN);
_Static_assert(
    sizeof(rva_container_data) <= DATA_BYTES &&
    _Alignof(rva_container_data) == DATA_ALIGN);
_Static_assert(
    sizeof(vmalloc_container) == KPAGE_SIZE &&
    _Alignof(vmalloc_container) == KPAGE_ALIGN);


void vmalloc_init_containers();

fva_node* get_new_fva_node();
rva_node* get_new_rva_node();

void free_fva_node(fva_node* node);
void free_rva_node(rva_node* node);


void vmalloc_containers_debug_fva();
void vmalloc_containers_debug_rva();
