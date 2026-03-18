#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>
#include <syscall.h>


noreturn void syscall_exit(uint64_t exit_code)
{
    syscall(exit_code, 0, 0, 0, 0, 0, SYSC_EXIT);

    __builtin_unreachable();
}


sysc_print_results syscall_print(const void* buf, size_t size)
{
    return syscall((uint64_t)buf, size, 0, 0, 0, 0, SYSC_PRINT);
}
