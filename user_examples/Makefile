# =========================
# Configuración obligatoria
# =========================

SRC ?= err

ifeq ($(SRC),err)
$(error SRC not defined. Use: make SRC=path/to/src)
endif


# =========================
# Toolchain
# =========================

CROSS   ?= aarch64-none-elf
CC      := $(CROSS)-gcc
LD      := $(CROSS)-ld
OBJCOPY := $(CROSS)-objcopy

# =========================
# Flags
# =========================

CFLAGS  := -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -O0
ASFLAGS := -ffreestanding
LDFLAGS := -T linker.ld -nostdlib

# =========================
# Sources
# =========================

SRC_DIR := $(SRC)

SRC_S   := $(shell find $(SRC_DIR) -name '*.S')
SRC_C   := $(shell find $(SRC_DIR) -name '*.c')
SRC_CPP := $(shell find $(SRC_DIR) -name '*.cpp')
SRC_RS  := $(shell find $(SRC_DIR) -name '*.rs')

OBJS := \
	$(SRC_S:.S=.o) \
	$(SRC_C:.c=.o) \
	$(SRC_CPP:.cpp=.o)

# =========================
# Output
# =========================

TARGET := test.elf

# =========================
# Rules
# =========================

all: $(TARGET)

$(TARGET): $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -fno-exceptions -fno-rtti -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean