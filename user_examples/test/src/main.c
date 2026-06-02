#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdthread.h>

extern void print_thid(uint64_t thid);

#define N 100

static void secondary_entry(uint64_t thid, uint64_t arg)
{
    print_thid(thid);

    atomic_int* flag = (void*)arg;
    atomic_store(flag, 1);

    thread_kill(thid);

    exit(1);
}

int main()
{
    atomic_int threads[N] = {[0 ... N - 1] = 0};

    for (size_t i = 0; i < N; i++) {
        thread_create(secondary_entry, (uintptr_t)&threads[i]);
    }

retry:
    yield();
    
    for (size_t i = 0; i < N; i++)
        if (atomic_load(&threads[i]) != 1)
            goto retry;

    exit(0);
}
