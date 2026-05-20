ENTRY(_start)

SECTIONS
{
	/*
	 * Load at 2M. Safer than 1M under UEFI firmware, and effectively
	 * guaranteed to be available under BIOS/GRUB.
	 */
	. = 2M;

	.text BLOCK(4K) : ALIGN(4K)
	{
		/*
		 * .multiboot must come first so the Multiboot header lands
		 * within the first 8 KiB of the image where GRUB looks for it.
		 */
		*(.multiboot)
		*(.text)
	}

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)

		/*
		 * Mark the end of the kernel image. Align to a page boundary
		 * so that a future physical memory allocator can hand out
		 * pages cleanly starting from here.
		 */
		. = ALIGN(4K);
		kernel_end = .;
	}
}
