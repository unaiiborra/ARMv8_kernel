#pragma once

#include <arm/mmu.h>
#include <kernel/lib/kvec.h>
#include <kernel/process/thread.h>
#include <lib/stdint.h>


typedef struct {
    v_uintptr knl_va; // kernel va
    v_uintptr usr_va; // user va
    size_t pages;
} proc_map_info;


typedef struct {
    kvec_T(proc_map_info) pmap_info;
    mmu_mapping usr_mapping;
} proc_map;


typedef struct {
    uint64 pthread_id_counter;
    size_t pthread_count;
    kvec_T(pthread*) pthreads;
} proc_threads;


typedef struct proc {
    uint64 pid;
    const char* PNAME;

    v_uintptr entry;

    proc_threads threads;

    proc_map map;
} proc;


void usr_proc_ctrl_init();

bool usr_proc_new(proc** out, void* elf, size_t elf_size, const char* pname);
