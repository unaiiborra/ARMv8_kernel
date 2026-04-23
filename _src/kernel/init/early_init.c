#include <kernel/init.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>


void kernel_early_init(void)
{
//    io_early_init();
    mm_early_init(); // returns to the kernel entry
}
