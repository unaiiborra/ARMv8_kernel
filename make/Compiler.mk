# Compiler
CROSS_COMPILE 	?= aarch64-none-elf-
CC 			= $(CROSS_COMPILE)gcc
ASM 		= $(CC)
CPP 		= $(CROSS_COMPILE)g++
LD 			= $(CROSS_COMPILE)ld
OBJCOPY 	= $(CROSS_COMPILE)objcopy
OBJDUMP 	= $(CROSS_COMPILE)objdump

# Rust
RUST		= cargo