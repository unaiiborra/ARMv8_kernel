SRC ?= err

MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))


ifeq ($(SRC),err)
$(error SRC not defined. Use: make SRC=path/to/src)
endif


CROSS   ?= aarch64-none-elf
CC      := $(CROSS)-gcc
LD      := $(CROSS)-ld
OBJCOPY := $(CROSS)-objcopy


CFLAGS  := -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -O0
ASFLAGS := -ffreestanding
LDFLAGS := -T linker.ld -nostdlib

SRC_DIR := $(MAKEFILE_DIR)$(SRC)
OBJ_DIR := $(SRC_DIR)/obj

SRC_S   := $(shell find $(SRC_DIR) -name '*.S')
SRC_C   := $(shell find $(SRC_DIR) -name '*.c')
SRC_CPP := $(shell find $(SRC_DIR) -name '*.cpp')
SRC_RS  := $(shell find $(SRC_DIR) -name '*.rs')

OBJS := \
	$(patsubst $(SRC_DIR)/%.S,$(OBJ_DIR)/%.o,$(SRC_S)) \
	$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_C)) \
	$(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_CPP))


TARGET := $(SRC)/$(notdir $(SRC)).elf


all: $(TARGET)

$(TARGET): $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fno-exceptions -fno-rtti -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean