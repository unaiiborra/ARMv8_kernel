set architecture aarch64
target remote localhost:2331
set print pretty on
monitor halt

load
set $pc = __ENTRY__
add-symbol-file __ELF_PATH__ __ENTRY__

break kernel_entry
layout split