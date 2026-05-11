SRC_S   = $(shell find $(SRC_DIR) -name '*.S')
SRC_C   = $(shell find $(SRC_DIR) -name '*.c')
SRC_CPP = $(shell find $(SRC_DIR) -name '*.cpp')
SRC_RS	= $(shell find $(SRC_DIR) -name '*.rs')

OBJ_S   = $(patsubst $(SRC_DIR)/%.S, $(OBJ_DIR)/__%.o, $(SRC_S))
OBJ_C   = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,  $(SRC_C))
OBJ_CPP = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_CPP))

OBJ 	= $(OBJ_S) $(OBJ_C) $(OBJ_CPP)

DISASM 	= $(shell find $(OBJ_DIR) -name '*.S')


# --- Output ---
KERNEL_FILE = kernel
TARGET      = $(BIN_DIR)/$(KERNEL_FILE).elf
BIN         = $(BIN_DIR)/$(KERNEL_FILE).bin
MAP         = $(MAP_DIR)/$(KERNEL_FILE).map



EMBED_OBJS :=
ifneq ($(EMBEDDED_BINARIES_PATH),)
    EMBED_SRCS := $(wildcard $(EMBEDDED_BINARIES_PATH)/*.bin $(EMBEDDED_BINARIES_PATH)/*.elf)
    EMBED_OBJS := $(patsubst $(EMBEDDED_BINARIES_PATH)/%, $(OBJ_DIR)/_binary_%.o, $(EMBED_SRCS))
endif