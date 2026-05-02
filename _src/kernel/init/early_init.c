#include <kernel/init.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <lib/stdattribute.h>



safe_early void kernel_early_init(void)
{
    //    io_early_init();  right now early logging is not implemented
    mm_early_init(); // returns to the kernel entry
}
