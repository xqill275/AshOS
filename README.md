# AshOS

A hobby x86 operating system built from scratch in C and x86 assembly.

## Overview

AshOS is a 32-bit x86 kernel built without any external libraries or operating system support. Everything — from memory management to the file system — is implemented from the ground up.

## Features

### Core Kernel
- **Multiboot compliant** — boots via GRUB on real hardware or QEMU
- **32-bit protected mode** — full x86 protected mode environment
- **GDT** — custom Global Descriptor Table with kernel code and data segments
- **IDT** — Interrupt Descriptor Table handling all 256 interrupts
- **CPU exception handling** — all 32 CPU exceptions caught and displayed with a descriptive error screen rather than silently resetting

### Memory Management
- **Physical memory manager** — bitmap-based page frame allocator tracking all available RAM
- **Virtual memory** — x86 paging enabled with a two-level page directory/table structure
- **Heap allocator** — `kmalloc` and `kfree` with first-fit allocation and block coalescing to reduce fragmentation

### Hardware Support
- **VGA text mode** — 80x25 colour text display with full colour support
- **Hardware cursor** — blinking cursor tracks typing position via VGA I/O ports
- **Keyboard driver** — full PS/2 keyboard support including shift, caps lock, and all standard keys
- **ATA PIO driver** — reads and writes 512-byte sectors to disk using ATA PIO mode

### Process Management
- **Multitasking** — cooperative/preemptive round-robin scheduler
- **Context switching** — full register save/restore via assembly
- **Process control** — create, run, and terminate kernel processes
- **PIT timer** — programmable interval timer drives the scheduler via IRQ0

### File System — AshFS
A custom file system designed for AshOS:
- **Persistent storage** — files survive reboots
- **Inode-based** — each file described by an inode storing name, size, and block pointers
- **Superblock** — disk layout described at sector 0
- **Up to 32 files** — with up to 8 data blocks (4KB) each
- **Operations** — create, read, write, delete, list

### Shell
An interactive command-line shell:

| Command | Description |
|---------|-------------|
| `help` | List available commands |
| `clear` | Clear the screen |
| `version` | Show OS version |
| `echo <text>` | Print text to screen |
| `ps` | List running processes |
| `meminfo` | Show memory usage |
| `ls` | List files on disk |
| `touch <file>` | Create a file |
| `write <file> <content>` | Write content to a file |
| `cat <file>` | Read and display a file |
| `rm <file>` | Delete a file |

## Project Structure

```
AshOS/
├── boot/
│   └── boot.s          # Multiboot header and kernel entry point
├── kernel/
│   ├── kernel.c        # Kernel entry point and VGA terminal
│   ├── gdt.c / gdt.s   # Global Descriptor Table
│   ├── idt.c / idt.s   # Interrupt Descriptor Table
│   ├── pmm.c           # Physical memory manager
│   ├── vmm.c           # Virtual memory manager
│   ├── heap.c          # Heap allocator (kmalloc/kfree)
│   ├── process.c / process.s # Process management and scheduler
│   ├── kprintf.c       # Kernel printf implementation
│   ├── shell.c         # Interactive shell
│   └── fs.c            # AshFS file system
├── drivers/
│   ├── keyboard.c      # PS/2 keyboard driver
│   └── ata.c           # ATA PIO disk driver
├── include/
│   └── *.h             # Header files
├── linker.ld           # Linker script
└── Makefile            # Build system
```

## Building and Running

### Requirements
- `i686-elf-gcc` cross compiler
- `i686-elf-as` assembler
- `qemu-system-i386`

### Build
```bash
make
```

### Run
```bash
# Create a blank disk image first (only needed once)
dd if=/dev/zero of=disk.img bs=512 count=2048

make run
```

### Clean
```bash
make clean
```

## Technical Details

| Property | Value |
|----------|-------|
| Architecture | x86 (32-bit) |
| Load address | 2MB |
| Stack size | 16KB |
| Heap size | 1MB (at 16MB virtual) |
| Page size | 4KB |
| Kernel stack per process | 4KB |
| Max processes | 16 |
| Bootloader | GRUB (Multiboot) |
| Disk interface | ATA PIO |
| File system | AshFS |

## Roadmap

- [ ] Userspace programs (ring 3)
- [ ] System calls
- [ ] ELF binary loader
- [ ] Per-process virtual address spaces
- [ ] More shell commands
- [ ] `/bin` style executable loading
