#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <syscall.h>


#define SIZE (4096 * 4)

int main()
{
    print("=== mmap / munmap demo ===\n");

    // Allocate
    int64_t map = syscall_mmap(
        0,
        SIZE,
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS,
        0,
        0);

    if (map < 0) {
        switch ((sysc_mmap_res_e)map) {
            case SYSC_MMAP_ERR:
                print("[ FAIL ] mmap: SYSC_MMAP_ERR\n");
                syscall_exit(1);
            case SYSC_MMAP_CFG_NOT_SUPPORTED:
                print("[ FAIL ] mmap: SYSC_MMAP_CFG_NOT_SUPPORTED\n");
                syscall_exit(1);
        }
    }

    print("[  OK  ] mmap allocated\n");

    // Write
    uint64_t*          addr = (uint64_t*)map;
    volatile uint64_t* test = (volatile uint64_t*)map;

    for (size_t i = 0; i < SIZE / sizeof(uint64_t); i++)
        addr[i] = i;

    // Verify
    for (size_t i = 0; i < SIZE / sizeof(uint64_t); i++) {
        if (test[i] != i) {
            print("[ FAIL ] r/w mismatch\n");
            syscall_exit(1);
        }
    }

    print("[  OK  ] r/w verified\n");

    // Unmap
    sysc_munmap_res_e unmap = syscall_unmap((uintptr_t)addr, SIZE);
    switch (unmap) {
        case SYSC_MUNMAP_OK:
            print("[  OK  ] munmap ok\n");
            break;
        case SYSC_MUNMAP_ERR:
            print("[ FAIL ] munmap: SYSC_MUNMAP_ERR\n");
            syscall_exit(1);
        case SYSC_MUNMAP_ALIGN_NOT_SUPPORTED:
            print("[ FAIL ] munmap: SYSC_MUNMAP_ALIGN_NOT_SUPPORTED\n");
            syscall_exit(1);
    }

    print("=== done ===\n");
    syscall_exit(0);
}