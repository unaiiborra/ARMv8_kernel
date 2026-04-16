#include "stdlib.h"

#include "syscall.h"


noreturn void exit(int status)
{
    syscall_exit(status);
}
