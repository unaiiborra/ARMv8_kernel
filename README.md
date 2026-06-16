# ARMv8 Kernel (AArch64)

A custom operating system kernel for ARMv8-A architecture, running on real hardware. The project explores the core components of a modern OS: virtual memory, userspace execution, hardware drivers, and process management. Built from scratch in C and ARMv8 assembly, with some components written in Rust.

Currently running on the **NXP i.MX8M Plus (FRDM i.MX 8M Plus, ARM Cortex-A53)**. A port to the **Raspberry Pi 5** and **Raspberry Pi Zero 2 W** is in planned.

---

## Features

### Memory Management

- ARMv8 translation table implementation (MMU) with 4 KiB granularity (other granularities are supported by the MMU module but untested)
- Kernel/user address space separation
- Kernel relocation to high virtual addresses
- Physical and virtual memory page allocators (`page allocator` and `vmalloc`)
- Multiple kernel dynamic memory allocator layers: `raw_kmalloc`, `kmalloc`, `cache_malloc`, and `umalloc` for userspace
- Direct hardware configuration via system registers (MAIR, TCR, SCTLR, etc.)

### Process Management and Execution

- Hardware exception handling (synchronous and asynchronous, EL1 and EL2)
- Simple panic handler for easier debugging
- ELF binary loader
- EL1 to EL0 (kernel to userspace) transition
- Basic syscall interface (`map`, `unmap`, `print`, `exit`)
- Preemptive round-robin scheduler
- Multithreading support

### Drivers

- UART
- ARM Generic Timer
- Generic Interrupt Controller (GICv3)
- Thermal Monitoring Unit

### Development Environment

- Runs on real hardware (FRDM i.MX8M Plus, ARM Cortex-A53)
- Booted via U-Boot
- GDB debugging via J-Link

---

## Project Structure

```
src/
├── arm/           # Low-level ARMv8 code: MMU, cache, exceptions, sysregs, TF-A/SMCCC
├── boot/          # Early boot: stack setup, BSS clearing, EL2 to EL1 transition
├── drivers/       # Hardware drivers (UART, GICv3, ARM timer, TMU)
├── kernel/        # Core kernel subsystems (see below)
│   ├── init/      # Early and main initialization sequence
│   ├── mm/        # Memory management: page allocator, vmalloc, kmalloc, ELF loader
│   ├── scheduler/ # Task and thread management, context switching
│   ├── syscall/   # Syscall dispatch and handlers
│   ├── exception/ # Exception and IRQ handling at EL0/EL1
│   ├── panic/     # Kernel panic with some ESR decoding
│   ├── io/        # UART-backed stdio and terminal buffers
│   └── devices/   # Device map and driver registration
├── lib/           # Shared utilities: spinlocks, memcpy, string formatting
userspace/         # Minimal C standard library and template for compiling userspace programs
user_examples/     # Example userspace programs and embedding tooling
```

---

## How It Works

### Boot

The kernel starts in assembly (`src/boot/`), sets up stacks, clears BSS, flushes the caches and performs the initial EL2 → EL1 transition and jumps to the C code (`src/kernel/kernel_entry.c`).

### Initialization

Once execution reaches C, the kernel performs early MMU initialization (see `src/arm/mmu/` and `src/kernel/mm/init`). During this stage, it sets up the translation tables and performs an initial setup of the memory allocators. It then relocates the kernel to high virtual addresses and jumps back to the kernel entry point, now executing from the higher virtual address space.

Once running in the higher virtual address space, the kernel completes memory subsystem initialization, bringing all allocators to a fully operational state. It then initializes device drivers and other subsystems, and finally transitions into the main scheduler loop.

### Memory

Memory management is organized in layers. At the lowest level, a buddy based physical page allocator manages raw memory pages in power of two sizes. On top of this, a virtual memory allocator handles virtual address space management.

Building on top of those, `raw_kmalloc` manages dynamic virtual allocations by mapping virtual pages to physical memory. Higher level allocators `kmalloc`, `cache_malloc`, and a userspace allocator (`umalloc`) are implemented on top of these layers, providing more convenient allocation mechanisms.

### Processes and Userspace

The kernel loads user programs from ELF binaries, maps them into a separate address space, and transitions to EL0 for execution. Currently, user binaries are embedded directly into the kernel binary as byte arrays. A filesystem is the planned replacement.

---

## Building

### Requirements

- `aarch64-none-elf` cross-compiler toolchain
- Rust with target `aarch64-unknown-none` (`rustup target add aarch64-unknown-none`)
- GNU Make

### Configuration

Copy `.env.example` to `.env` and adjust to your setup:

```dotenv
CROSS_COMPILE      = aarch64-none-elf-
CROSS_COMPILE_PATH = /usr/lib/gcc/aarch64-none-elf/15.2.1

RS_TARGET = aarch64-unknown-none

# Build profile: gdb (debug) or release (default)
CONFIG = release
```

**Build profiles:**

- `std` - `-O2`, no debug symbols (default)
- `gdb` - `-O0 -g3`, defines `-DDEBUG`

Individual settings (`DEBUG`, `OPT_LEVEL`, `MCPU`) can be overridden per-variable regardless of profile see `.env.example` for details.

### Building

```bash
make
```

The output kernel image will be at `bin/kernel.elf`.

---

## Booting

The kernel is loaded via U-Boot. After transferring `bin/kernel.bin` or `bin/kernel.elf` to the board, boot it from the U-Boot prompt:

#### Raw binary

```bash
mmc dev 1
fatload mmc 1:1 0x40200000 kernel.bin
go 0x40200000
```

#### ELF

```bash
mmc dev 1
fatload mmc 1:1 0x50000000 kernel.elf
bootelf -p 0x50000000
```

To load and debug directly via J-Link:

```bash
make
./scripts/debug_jlink_gdb.sh
```

---

## Userspace

A minimal C and Rust standard library (`userspace/stl/`) and a program template (`userspace/template/`) are provided for writing userspace code. To create a new userspace program, copy the template and implement a `main()` entry point, either in rust (with #[unsafe(no_mangle)]) or c. Make sure to update the path to the stl crate at the Cargo.toml.

Available STL headers: `stdio.h`, `stdlib.h`, `syscall.h`, `stdthread.h`.

### Embedding a userspace binary
 
Since no filesystem is implemented yet, userspace binaries are embedded directly into the kernel binary at link time. Place your compiled ELF (or any binary) in the directory defined by `EMBEDDED_BINARIES_PATH` in your `.env`:
 
```dotenv
EMBEDDED_BINARIES_PATH = embedded
```
 
The build system will automatically discover all the files in that directory and embed them as raw bytes in the `.rodata` section.
 
To access an embedded binary from kernel code, include the header and use the provided macros:
 
```c
#include <kernel/embedded_binary.h>
 
// File: embedded/my_program.elf
// Macro name: replace '.' and '-' with '_' → my_program_elf
 
elf_load(task, EMBEDDED_BINARY(my_program_elf), EMBEDDED_BINARY_SIZE(my_program_elf), &entry);
```
 
The macro name is derived from the filename with `.` and `-` replaced by `_`:
 
| File | Macro name |
|---|---|
| `embedded/hello_world.elf` | `hello_world_elf` |
| `embedded/firmware.bin` | `firmware_bin` |
 
Available macros:
 
```c
EMBEDDED_BINARY(name)       // const void*    — pointer to the start of the binary
EMBEDDED_BINARY_START(name) // const uint8_t* — same as above
EMBEDDED_BINARY_END(name)   // const uint8_t* — pointer past the last byte
EMBEDDED_BINARY_SIZE(name)  // size_t         — size in bytes (end - start)
```
 
This replaces the previous workflow of manually running `bin_to_array.py` and adding generated C arrays to the build. This also will be replaced once a proper filesystem is implemented.
 

## Roadmap

- [ ] Complete kernel thread support (scheduler currently handles user threads only)
- [X] Full preemptive multithreading
- [ ] FAT32 filesystem to replace binary embedding with proper file loading
- [ ] Port to Raspberry Pi 5
- [ ] Port to Raspberry Pi Zero 2 W

---

## License

This project is licensed under the **GNU General Public License v2.0**. See [LICENSE](LICENSE) for details.
