BIN_DIR    			:= bin
SRC_DIR     		:= _src
INCLUDE_DIR 		:= _include
MAP_DIR				:= $(BIN_DIR)
OBJ_DIR				:= $(BIN_DIR)/obj
RS_LIB_DIR  		:= target/$(RS_TARGET)/release/libimx8mp_kernel.a
CROSS_COMPILE_PATH	?= /usr/lib/gcc/aarch64-none-elf/15.2.1