#pragma once


#include <arm/mmu.h>
#include <kernel/hardware.h>
#include <kernel/mm/umalloc.h>
#include <lib/lock/spinlock.h>
#include <lib/stdbitfield.h>
#include <lib/stdint.h>

#include "arm/exceptions/exceptions.h"
#include "kernel/lib/kvec.h"


void scheduler_loop_cpu_enter();
void scheduler_loop_cpu_exit();


void scheduler_ectx_save(arm_exception_ctx* ectx);
void schedurer_ectx_restore(arm_exception_ctx* ectx);


/* --- Tasks --- */

typedef struct usr_region_node {
    struct usr_region_node* next;
    usr_region region;
} usr_region_node;


typedef struct utask {
    uint64 task_uid;
    const char* task_name;
    spinlock_t lock;

    mmu_mapping mapping;
    usr_region_node* regions;
    kvec_T(thread*) threads;
} utask;


typedef struct {
    uint64 task_uid;
    const char* task_name;
    spinlock_t lock;
    void (*fn)(void* args);
    struct thread* threads[NUM_CORES];
} ktask;


/* --- Threads --- */

typedef enum {
    KERNEL_THREAD,
    USER_THREAD,
} thread_type;


typedef enum {
    THREAD_NEW,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_SLEEPING,
    THREAD_DEAD,
} thread_state;

typedef struct thread {
    uint64 th_uid;

    union {
        ktask* ktask;
        utask* utask;
    } task;

    uint64 sp;
    uint64 pc;
    arm_exception_ctx ctx;

    uint64 last_access_time_us;
    uint32 th_flags;
} thread;

