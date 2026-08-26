# CSE231: Programming Assignment 1 — SimpleLoader and Bootloader

Name: Arpit Shukla
Roll Number: 2025105
Private GitHub Repository: https://github.com/Arpit05057/OS-PA-1

# Overview

This assignment contains two parts:

1. **Part A — SimpleLoader:** A 32-bit ELF executable is loaded into memory and its `_start` function is executed using a C-based loader.
2. **Part B — Bootloader:** A 16-bit x86 bootloader is assembled into a 512-byte boot sector and executed using QEMU.

# Part A — SimpleLoader

## Files

1. `loader.c` — Implements the SimpleLoader and cleanup routine.
2. `loader.h` — Contains the required headers and function declarations.
3. `factorial.c` — Test executable used to verify the loader.
4. `Makefile` — Contains compilation and cleanup commands.

The provided `loader.h` was kept unchanged.

## ELF Structures

The loader uses `Elf32_Ehdr` and `Elf32_Phdr` from `<elf.h>`.

### Elf32_Ehdr

1. `e_ident` — Used to check the ELF magic number and ELF class.
2. `e_type` — Specifies the ELF file type and is checked for `ET_EXEC`.
3. `e_entry` — Contains the virtual address of the entry point.
4. `e_phoff` — Gives the offset of the Program Header Table.
5. `e_phnum` — Gives the number of program headers.

### Elf32_Phdr

1. `p_type` — Specifies the segment type; the loader searches for `PT_LOAD`.
2. `p_offset` — Gives the segment offset inside the ELF file.
3. `p_vaddr` — Gives the virtual address of the segment.
4. `p_filesz` — Gives the amount of segment data stored in the file.
5. `p_memsz` — Gives the total memory required by the segment.

## SimpleLoader Implementation

The main loading operation is performed by `load_and_run_elf(char **exe)`.

### Step 1 — Open and Read

The executable path is obtained from `exe[0]`.

The loader opens the executable, determines its size, reads it into memory and checks that each operation succeeds before continuing.

### Step 2 — Validate the ELF

The loaded file is interpreted as an ELF header. The loader checks:

1. The ELF magic number.
2. `ELFCLASS32` to confirm a 32-bit ELF.
3. `e_type == ET_EXEC` to confirm an executable file.
4. `e_phoff` and `e_phnum` to locate the Program Header Table.

If a check fails, an error is displayed and execution stops.

### Step 3 — Find PT_LOAD

The loader locates the Program Header Table using `e_phoff` and searches for a `PT_LOAD` segment containing the entry point:

```text
p_type == PT_LOAD
e_entry >= p_vaddr
e_entry < p_vaddr + p_memsz
```

If no suitable segment is found, the loader exits with an error.

### Step 4 — Map and Copy the Segment

The loader maps `p_memsz` bytes into memory and copies the segment data from the ELF file.

```text
p_filesz → data present in the ELF file
p_memsz  → total memory required by the segment
```

### Step 5 — Calculate the Entry Point

The actual entry point is calculated using the mapped segment address and the offset of `e_entry` within the selected segment. Execution is then transferred to the executable's `_start` function.

### Step 6 — Print the Result

The loader prints the return value from `_start`.

The factorial program sets `num = 5`, calculates `factorial(5)` and returns `120`.

Expected output:

```text
User _start return value = 120
```

## Cleanup

After execution, `loader_cleanup()` performs:

1. `munmap()` — Releases the mapped segment.
2. `free()` — Releases the ELF file buffer.
3. `close()` — Closes the file descriptor.

The pointers are reset to `NULL` and the descriptor to `-1`.

## Error Handling

The implementation checks:

1. Command-line arguments.
2. File opening.
3. File size and seeking.
4. Memory allocation.
5. Complete file reading.
6. ELF magic number.
7. 32-bit ELF class.
8. Executable type.
9. Program Header Table.
10. `PT_LOAD` selection.
11. `mmap()`.

Each failure produces an appropriate error message before exiting.

## Build and Run

Build the project:

```bash
make
```

The factorial test case uses:

```bash
gcc -m32 -no-pie -nostdlib -o factorial factorial.c
```

The loader uses:

```bash
gcc -m32 -o loader loader.c
```

Run:

```bash
./loader ./factorial
```

Expected result:

```text
User _start return value = 120
```

Clean generated files:

```bash
make clean
```

## SimpleLoader Execution Flow

Open ELF → Get file size → Read ELF into memory → Validate ELF → Find Program Header Table → Find PT_LOAD containing e_entry → Map p_memsz → Copy p_filesz → Calculate entry point → Execute _start → Print result → Cleanup

# Part B — x86 Bootloader

## Files

1. `boot.asm` — Source code of the bootloader.
2. `boot.bin` — Raw 512-byte bootloader binary.

## Overview

The bootloader is a 16-bit x86 program designed to fit inside a single boot sector. It displays the message:

```text
Hello from my bootloader!
```

NASM is used to create the raw binary and QEMU is used to execute it.

## Initialization

The bootloader begins with:

1. `bits 16` specifies 16-bit x86 instructions.
2. `org 0x7c00` specifies the address where the bootloader is expected to execute.

## Step 1 — Point to the Message

The `SI` register stores the address of the message and is used to read it character by character.

## Step 2 — Read Characters

1. `lodsb` loads one byte from `SI` into `AL` and advances `SI`.
2. `cmp al, 0` checks for the null terminator.
3. If the value is zero, execution moves to `hang`.

The message is stored as:

```asm
message:
    db "Hello from my bootloader!", 0
```

## Step 3 — Display Characters

`AH = 0x0E` selects the BIOS teletype output service. The character stored in `AL` is displayed on the screen.

The bootloader then returns to `print` and processes the next character.

## Step 4 — Halt

After the message is complete:

`cli` disables interrupts, `hlt` halts the processor and the jump keeps execution in the halt loop.

## 512-Byte Boot Sector

The bootloader reserves space and adds the boot signature using:

Therefore:

```text
510 bytes → code, message and padding
2 bytes   → boot signature
Total     → 512 bytes
```

The generated `boot.bin` must therefore be exactly 512 bytes.

## 0x7C00

`0x7C00` is the address associated with the BIOS loading location of the boot sector. The source uses:

```asm
org 0x7c00
```

This allows addresses to be calculated relative to the location where the bootloader executes.

## Boot Signature — 0x55AA

The boot sector ends with:

```asm
dw 0xaa55
```

Due to little-endian byte ordering, the bytes appear as:

```text
55 AA
```

This signature identifies the sector as a valid boot sector.

## Build and Run

Install NASM and QEMU:

```bash
sudo apt update
sudo apt install nasm qemu-system-x86
```

Assemble the bootloader:

```bash
nasm -f bin boot.asm -o boot.bin
```

Check the binary size:

```bash
ls -l boot.bin
```

The size should be `512` bytes.

Run using QEMU:

```bash
qemu-system-i386 -drive format=raw,file=boot.bin
```

Expected output:

```text
Hello from my bootloader!
```

## Role of QEMU

QEMU provides a virtual x86 machine for testing the bootloader. It treats `boot.bin` as a raw boot device and allows the boot sector to be loaded and executed without physical hardware.

## Bootloader Execution Flow

BIOS/QEMU → Load boot sector at 0x7C00 → Start execution → SI → message → lodsb reads character → Check for NULL → int 0x10 displays character → Repeat → cli → hlt → hang

# Overall Assignment Working

PART A — SimpleLoader

ELF File ↓ Read → Validate → Find PT_LOAD ↓ Map → Copy → Calculate Entry Point ↓ Execute _start ↓ Return Value = 120

PART B — Bootloader

boot.bin → BIOS/QEMU → Load at 0x7C00 → Execute 16-bit Code → BIOS int 0x10 → Display Message → Halt

# Conclusion

Part A demonstrates how a 32-bit ELF executable can be manually loaded and executed by interpreting its ELF and program headers. The SimpleLoader successfully executes the factorial program and obtains the return value `120`.

Part B demonstrates the basic x86 boot process using a 16-bit boot sector. The bootloader fits within 512 bytes, uses `0x7C00` as its execution origin, ends with the `0x55AA` boot signature and displays a message through the BIOS video service.

Together, both parts demonstrate the fundamental process of loading executable code, transferring control to it and executing low-level x86 code.
