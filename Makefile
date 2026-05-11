-include .env
include make/configs/CONFIGS.mk
include make/Flags.mk
include make/Compiler.mk
include make/Folders.mk
include make/Files.mk

all: $(BIN)


# Embedded binary objects
.ONESHELL:

$(OBJ_DIR)/_binary_%.o: $(EMBEDDED_BINARIES_PATH)/%
	mkdir -p $(dir $@)
	$(eval SYM := $(shell echo '$*' | tr '.-/ ' '_'))
	cat > $(OBJ_DIR)/_binary_$*.S <<'EOF'
	.section .rodata._embed_$(SYM),"aG",%progbits,_embed_$(SYM),comdat
	.global _embed_$(SYM)_start
	.global _embed_$(SYM)_end
	.align 6
	_embed_$(SYM)_start:
		.incbin "$<"
	_embed_$(SYM)_end:
	EOF
	$(ASM) $(ASM_FLAGS) -c $(OBJ_DIR)/_binary_$*.S -o $@


# Assembly files
$(OBJ_DIR)/__%.o: $(SRC_DIR)/%.S
	mkdir -p $(dir $@)
	$(ASM) $(ASM_FLAGS) -c $< -o $@
	

# C files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) -c $< -o $@


# Cpp files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CPP) $(CPP_FLAGS) -c $< -o $@


# Rust files TODO: Allow using debug build + gdb
$(OBJ_DIR)/rslib.a: Cargo.toml _src/*.rs
	$(RUST) build --release --target $(RS_TARGET)
	mkdir -p $(OBJ_DIR)
	cp $(RS_LIB_DIR) $(OBJ_DIR)/rslib.a


# Link
$(TARGET): $(OBJ_DIR)/rslib.a $(OBJ) $(EMBED_OBJS)
	mkdir -p $(dir $@)
	$(LD) $(LD_FLAGS) -o $@ \
	$(EMBED_OBJS) \
    $(OBJ) \
    $(OBJ_DIR)/rslib.a \


# BIN
$(BIN): $(TARGET)
	mkdir -p $(dir $@)
	$(OBJCOPY) -O binary $(TARGET) $(BIN)
	


clean:
	rm -rf $(BIN_DIR)/* target


disasm: $(OBJ)
	@for o in $(OBJ); do \
	    $(OBJDUMP) -D $$o > $$o.S; \
	done

full-disasm: $(TARGET)
	$(OBJDUMP) -D -S $(TARGET) > $(TARGET).dump.S