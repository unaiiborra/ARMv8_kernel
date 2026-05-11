TARGET    ?= out.elf
CROSS     ?= aarch64-none-elf
CC        := $(CROSS)-gcc

LD         = linker.ld
INCLUDE   ?= include

RS_TARGET := aarch64-unknown-none
RS_LIB    := target/$(RS_TARGET)/release/libapp.a

CFLAGS += -ffreestanding -Wall -Wextra -O2 \
          -Istl/include \
          -I$(INCLUDE) \
          -g$(G)

LDFLAGS ?= -T $(LD) -nostdlib

SRC_DIR ?= src
OBJ_DIR ?= _obj

C_SRC  := $(wildcard $(SRC_DIR)/*.c)
C_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SRC))

all: $(TARGET)

$(RS_LIB): $(wildcard src/*.rs)
	cargo build --release --target $(RS_TARGET)

$(TARGET): $(C_OBJS) $(RS_LIB)
	$(CC) $(C_OBJS) $(RS_LIB) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	cargo clean

.PHONY: all clean