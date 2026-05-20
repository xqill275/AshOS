.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text
.global _start
.type _start, @function
_start:
	mov $stack_top, %esp

	/*
	Pass the multiboot magic number and info pointer to kernel_main.
	- eax contains the multiboot magic number (0x2BADB002 if loaded by a
	  compliant bootloader) which can be checked in the kernel to confirm
	  we were booted correctly.
	- ebx contains a pointer to the multiboot info structure, which holds
	  memory map data, boot device info, and more. We requested the memory
	  map via the MEMINFO flag so we should make use of it.
	We push ebx first as it is the second argument (cdecl pushes right-to-left).
	*/
	push %ebx
	push %eax

	call kernel_main

	/*
	kernel_main should never return. If it does, disable interrupts and
	halt. The jmp catches any spurious wakeups from NMIs.
	*/
	cli
1:	hlt
	jmp 1b

.size _start, . - _start
