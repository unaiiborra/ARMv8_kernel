#pragma once

#include <arm/mmu.h>
#include <kernel/lib/kvec.h>
#include <lib/mem.h>
#include <lib/stdint.h>


typedef struct {
    v_uintptr knl_va; // kernel va
    v_uintptr usr_va; // user va
    uint32 pages;
    bool read;
    bool write;
    bool execute;
} proc_map_info;


typedef struct {
    kvec_T(proc_map_info) pmap_info;
    mmu_mapping usr_mapping;
} proc_map;


typedef struct {
    uint64 pthread_id_counter;
    size_t pthread_count;
    kvec_T(thread*) pthreads;
} proc_threads;


typedef struct proc {
    uint64 pid;
    const char* PNAME;

    v_uintptr entry;

    proc_threads threads;

    proc_map map;
} proc;


void usr_proc_ctrl_init();

bool usr_proc_new(
    proc** out,
    void* elf,
    size_t elf_size,
    const char* pname,
    bool acquire_thread);


bool proc_get_usrmap_info(proc* pr, v_uintptr usrva, proc_map_info* out);
