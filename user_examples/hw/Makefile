TARGET ?= out.elf

CROSS ?= aarch64-none-elf

CC := $(CROSS)-gcc

LD 		= linker.ld
STL_DIR ?= stl
INCLUDE ?= include

CFLAGS += -ffreestanding -Wall -Wextra -O2 \
          -I$(STL_DIR)/include \
          -I$(INCLUDE)

STL_LIB := $(STL_DIR)/libstl.a

LDFLAGS ?= -T $(LD) -nostdlib

SRC_DIR ?= src
OBJ_DIR ?= _obj

SRC  := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(STL_LIB) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean