#include <kernel/io/stdio.h>

#include "exception_class.h"


void svc_aarch64_handler()
{
    kprint("\n\n SVC call received \n\r");
}
