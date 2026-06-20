# release | debug
PROFILE   ?= debug

TARGET    ?= out.elf
CROSS     ?= aarch64-none-elf
CC        := $(CROSS)-gcc
CXX       := $(CROSS)-g++
LD        := $(CROSS)-gcc

LINKER_SCRIPT ?= linker.ld
INCLUDE       ?= include

RS_TARGET := aarch64-unknown-none

CXXFLAGS += -ffreestanding -Wall -Wextra \
            -Istl_include \
            -I$(INCLUDE)

CFLAGS   += -ffreestanding -Wall -Wextra \
            -Istl_include \
            -I$(INCLUDE)

LDFLAGS  ?= -T $(LINKER_SCRIPT) -nostdlib

ifeq ($(PROFILE), release)
    RS_PROFILE   := --release
    RS_LIB       := target/$(RS_TARGET)/release/libapp.a
    CFLAGS       += -O2
    CXXFLAGS     += -O2
else ifeq ($(PROFILE), debug)
    RS_PROFILE   :=
    RS_LIB       := target/$(RS_TARGET)/debug/libapp.a
    CFLAGS       += -O0 -g -DDEBUG=1 -DTESTING=1
    CXXFLAGS     += -O0 -g -DDEBUG=1 -DTESTING=1
    LDFLAGS      += -g
else
    $(error Unknown PROFILE '$(PROFILE)'. Use release or debug)
endif

SRC_DIR ?= src
OBJ_DIR ?= _obj

C_SRC    := $(wildcard $(SRC_DIR)/*.c)
CPP_SRC  := $(wildcard $(SRC_DIR)/*.cpp)
C_OBJS   := $(patsubst $(SRC_DIR)/%.c,  $(OBJ_DIR)/%.o, $(C_SRC))
CPP_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o, $(CPP_SRC))
ALL_OBJS := $(C_OBJS) $(CPP_OBJS)

all: $(TARGET)

$(RS_LIB): $(wildcard src/*.rs) Cargo.toml
	cargo build $(RS_PROFILE) --target $(RS_TARGET)

$(TARGET): $(ALL_OBJS) $(RS_LIB)
	$(LD) $(ALL_OBJS) $(RS_LIB) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	cargo clean

.PHONY: all clean
