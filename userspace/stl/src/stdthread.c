#include "stdthread.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "syscall.h"



bool thread_create(thread_start_t entry, void* arg)
{
    sysc_spawn_res result =
        syscall_spawn((spawn_fn_t)(void*)entry, (uint64_t)arg);

    return result == SYSC_SPAWN_RES_OK;
}


bool thread_exit(thread_id_t thid)
{
    sysc_kill_res result = syscall_kill(thid);

    return result == SYSC_KILL_OK;
}


void thread_yield()
{
    syscall_yield();
}