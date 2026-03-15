#pragma once

#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>

#include "../containers/containers.h"
#include "../vmalloc.h"
#include "kernel/mm.h"
#include "lib/stdbitfield.h"

typedef bitfield64 mdt_bf;

typedef struct vmalloc_pa_mdt {
    struct vmalloc_pa_mdt* next;
    vmalloc_pa_info info;
} vmalloc_pa_mdt;


struct vmalloc_mdt_container;

typedef struct {
    struct vmalloc_mdt_container* next;
    struct vmalloc_mdt_container* prev;
} vmalloc_mdt_container_hdr;


#define PA_MDT_CONTAINER_NODES \
    ((KPAGE_SIZE - sizeof(vmalloc_mdt_container_hdr)) / sizeof(vmalloc_pa_mdt))
#define PA_MDT_BF_COUNT BITFIELD_COUNT_FOR(PA_MDT_CONTAINER_NODES, mdt_bf)

typedef struct vmalloc_mdt_container {
    _Alignas(KPAGE_ALIGN) vmalloc_mdt_container_hdr hdr;

    bitfield64 reserved_entries[PA_MDT_BF_COUNT];
    vmalloc_pa_mdt entries[PA_MDT_CONTAINER_NODES];
} vmalloc_mdt_container;

_Static_assert(
    sizeof(vmalloc_mdt_container) <= KPAGE_SIZE,
    "vmalloc_mdt_container size must fit in one kernel page");

_Static_assert(
    _Alignof(vmalloc_mdt_container) == KPAGE_ALIGN,
    "vmalloc_mdt_container alignment must match KPAGE_ALIGN");

_Static_assert(
    BITFIELD_CAPACITY(mdt_bf) * PA_MDT_BF_COUNT >= PA_MDT_CONTAINER_NODES,
    "mdt_bf bitfield does not cover PA_MDT_CONTAINER_NODES");


void vmalloc_pa_mdt_init();

void vmalloc_pa_mdt_push(rva_node* n, size_t o, puintptr_t pa, vuintptr_t va);
void vmalloc_pa_mdt_free(rva_node* n);
