#include <stdalign.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <stdthread.h>

#define N 5

typedef struct {
    alignas(64) atomic_bool finished;
} finishline_t;


static finishline_t finishline[N];


static size_t u64_to_str(uint64_t n, char* buf)
{
    char   tmp[20];
    size_t len = 0;

    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }

    while (n > 0) {
        tmp[len++] = '0' + (n % 10);
        n /= 10;
    }

    for (size_t i = 0; i < len; i++)
        buf[i] = tmp[len - 1 - i];

    buf[len] = '\0';
    return len;
}


static noreturn void secondary_thread_hello(thread_id_t thid, void* arg)
{
    char id_str[21];
    char arg_str[21];
    u64_to_str(thid, id_str);
    u64_to_str((uint64_t)arg, arg_str);

    print("Im the thread ");
    print(id_str);
    print(" with arg ");
    print(arg_str);
    print("\n\r");


    atomic_store(&finishline[(uint64_t)arg].finished, true);

    bool res = thread_exit(thid);

    if (res)
        print(
            "secondary_thread_hello: received exit true but still executing!");
    else
        print("secondary_thread_hello: received exit false");


    exit(1);
}


int main()
{
    for (size_t i = 0; i < N; i++)
        atomic_init(&finishline[i].finished, false);

    for (size_t i = 1; i < N; i++) {
        thread_create(secondary_thread_hello, (void*)i);
    }

    print("Im the main thread\n\r");

    atomic_store(&finishline[0].finished, true);

retry:

    for (size_t i = 0; i < N; i++)
        if (!atomic_load(&finishline[i].finished)) {
            thread_yield();
            goto retry;
        }

    return 0;
}