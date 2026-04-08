#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <syscall.h>


static noreturn void th_spawn(uint64_t thid, uint64_t arg)
{
    (void)arg;

    char text[] = "Im the thread | with arg &\n\r";

    // NOTE: not good code, its just a simple example
    for (size_t i = 0; i < sizeof(text); i++) {
        if (text[i] == '|')
            text[i] = thid + '0';

        if (text[i] == '&')
            text[i] = arg + '0';
    }

    print(text);


    while (1)
        syscall_yield();
}


int main()
{
    uint64_t idx = 0;

    for (size_t i = 0; i < 5; i++) {
        sysc_spawn_res res = syscall_spawn(th_spawn, idx++);

        if (res != SYSC_SPAWN_RES_OK) {
            print("SYSC_SPAWN_RES_OK not received!\n\r");
            exit(1);
        }
    }

    print("Im the main thread\n\r");

    for (size_t i = 0; i < 500; i++)
        syscall_yield();


    return 0;
}