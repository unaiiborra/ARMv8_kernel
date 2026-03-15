include make/Folders.mk

OPT_LEVEL   ?= -O2
DEFINES     ?=

MARCH       ?= armv8-a
MCPU        ?= cortex-a53+simd
RS_TARGET	=  aarch64-unknown-none
CSTD		:= gnu23
CPPSTD		:= gnu++20 

ASM_FLAGS   = $(DEFINES) -I$(INCLUDE_DIR)

CX_FLAGS 	= $(OPT_LEVEL) $(DEFINES) -mgeneral-regs-only -Wall -Wextra -Werror -ffreestanding -nostdlib -nostartfiles -I$(INCLUDE_DIR) -march=$(MARCH) -mcpu=$(MCPU) -nostdinc -I$(COMPILER_DIR)/include

C_FLAGS     = $(CX_FLAGS) -x c -std=$(CSTD)
CPP_FLAGS   = $(CX_FLAGS) -x c++ -std=$(CPPSTD)

LD_FLAGS    = -T linker.ld -Map $(MAP)


$(OBJ_DIR)/drivers/%.o: C_FLAGS += -DDRIVERS
$(OBJ_DIR)/kernel/%.o: C_FLAGS += -DKERNEL
$(OBJ_DIR)/lib/%.o: C_FLAGS += -DLIB
$(OBJ_DIR)/boot/%.o: C_FLAGS += -DBOOT
$(OBJ_DIR)/arm/%.o: C_FLAGS += -DARM