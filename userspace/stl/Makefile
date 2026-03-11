CROSS   ?= aarch64-none-elf
CC      := $(CROSS)-gcc
AR      := $(CROSS)-ar

INCLUDE_DIR := include
SRC_DIR     := src
OBJ_DIR     := _obj

CFLAGS  := -ffreestanding -Wall -Wextra -Werror -O2 -I$(INCLUDE_DIR)
ASFLAGS := -ffreestanding -I$(INCLUDE_DIR)

SRC_S   := $(shell find $(SRC_DIR) -name '*.S')
SRC_C   := $(shell find $(SRC_DIR) -name '*.c')
SRC_CPP := $(shell find $(SRC_DIR) -name '*.cpp')

OBJS := \
	$(patsubst $(SRC_DIR)/%.S,$(OBJ_DIR)/%_S.o,$(SRC_S)) \
	$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%_C.o,$(SRC_C)) \
	$(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%_CPP.o,$(SRC_CPP))

TARGET := libstl.a


all: $(TARGET)


$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	mkdir -p ../template/stl/
	cp $(TARGET) ../template/stl/$(TARGET)
	cp -r $(INCLUDE_DIR) ../template/stl/

$(OBJ_DIR)/%_S.o: $(SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(OBJ_DIR)/%_C.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%_CPP.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fno-exceptions -fno-rtti -c $< -o $@
	
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	rm -rf ../template/stl/*


.PHONY: all clean