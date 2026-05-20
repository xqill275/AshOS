#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__i386__)
#error "This kernel needs to be compiled with an ix86-elf compiler"
#endif

/* ============================================================
   Multiboot
   ============================================================ */

#define MULTIBOOT_MAGIC 0x2BADB002

/*
 * Partial multiboot info structure. We only define the fields we
 * actually use. The full spec has many more.
 */
typedef struct {
	uint32_t flags;
	uint32_t mem_lower;  /* kilobytes of lower memory (below 1M) */
	uint32_t mem_upper;  /* kilobytes of upper memory (above 1M) */
} multiboot_info_t;

/* ============================================================
   VGA text mode
   ============================================================ */

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  ((uint16_t*)0xB8000)

enum vga_color {
	VGA_COLOR_BLACK        = 0,
	VGA_COLOR_BLUE         = 1,
	VGA_COLOR_GREEN        = 2,
	VGA_COLOR_CYAN         = 3,
	VGA_COLOR_RED          = 4,
	VGA_COLOR_MAGENTA      = 5,
	VGA_COLOR_BROWN        = 6,
	VGA_COLOR_LIGHT_GREY   = 7,
	VGA_COLOR_DARK_GREY    = 8,
	VGA_COLOR_LIGHT_BLUE   = 9,
	VGA_COLOR_LIGHT_GREEN  = 10,
	VGA_COLOR_LIGHT_CYAN   = 11,
	VGA_COLOR_LIGHT_RED    = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN  = 14,
	VGA_COLOR_WHITE        = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
	return (uint16_t)uc | (uint16_t)color << 8;
}

/* ============================================================
   Terminal state
   ============================================================ */

static size_t    terminal_row;
static size_t    terminal_column;
static uint8_t   terminal_color;
static uint16_t* terminal_buffer;

void terminal_initialize(void)
{
	terminal_row    = 0;
	terminal_column = 0;
	terminal_color  = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;

	for (size_t y = 0; y < VGA_HEIGHT; y++)
		for (size_t x = 0; x < VGA_WIDTH; x++)
			terminal_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
}

void terminal_setcolor(uint8_t color)
{
	terminal_color = color;
}

static void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	terminal_buffer[y * VGA_WIDTH + x] = vga_entry(c, color);
}

/*
 * Scroll the terminal up by one line.
 * Each row is copied to the row above it, then the bottom row is cleared.
 */
static void terminal_scroll(void)
{
	for (size_t y = 0; y < VGA_HEIGHT - 1; y++)
		for (size_t x = 0; x < VGA_WIDTH; x++)
			terminal_buffer[y * VGA_WIDTH + x] =
				terminal_buffer[(y + 1) * VGA_WIDTH + x];

	/* Clear the new bottom row */
	for (size_t x = 0; x < VGA_WIDTH; x++)
		terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
			vga_entry(' ', terminal_color);
}

void terminal_putchar(char c)
{
	if (c == '\n') {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) {
			terminal_scroll();
			terminal_row = VGA_HEIGHT - 1;
		}
		return;
	}

	if (c == '\r') {
		terminal_column = 0;
		return;
	}

	if (c == '\t') {
		/* Advance to the next 8-column tab stop */
		terminal_column = (terminal_column + 8) & ~(size_t)7;
		if (terminal_column >= VGA_WIDTH) {
			terminal_column = 0;
			if (++terminal_row == VGA_HEIGHT) {
				terminal_scroll();
				terminal_row = VGA_HEIGHT - 1;
			}
		}
		return;
	}

	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);

	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) {
			terminal_scroll();
			terminal_row = VGA_HEIGHT - 1;
		}
	}
}

void terminal_write(const char* data, size_t size)
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

/* ============================================================
   Minimal stdlib replacements
   ============================================================ */

size_t strlen(const char* str)
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void terminal_writestring(const char* data)
{
	terminal_write(data, strlen(data));
}

/*
 * Write an unsigned 32-bit integer as a decimal string to the terminal.
 * Useful for printing numbers without a full printf implementation.
 */
void terminal_writeuint(uint32_t n)
{
	if (n == 0) {
		terminal_putchar('0');
		return;
	}

	char buf[10]; /* max 10 digits for uint32 */
	int  i = 0;

	while (n > 0) {
		buf[i++] = '0' + (n % 10);
		n /= 10;
	}

	/* digits are in reverse order */
	while (i-- > 0)
		terminal_putchar(buf[i]);
}

/* ============================================================
   Kernel entry point
   ============================================================ */

/*
 * kernel_main is called from boot.s.
 * magic  - should equal MULTIBOOT_MAGIC (0x2BADB002)
 * mb_info - pointer to the multiboot info structure provided by GRUB
 */
void kernel_main(uint32_t magic, multiboot_info_t* mb_info)
{
	terminal_initialize();

	/* Verify we were booted by a Multiboot-compliant bootloader */
	if (magic != MULTIBOOT_MAGIC) {
		terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
		terminal_writestring("ERROR: Not loaded by a multiboot bootloader!\n");
		return;
	}

	/* Print a banner */
	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
	terminal_writestring("=== AshOS V200526 ===\n");

	terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
	terminal_writestring("Booted successfully via GRUB.\n\n");

	/* Print memory info if the bootloader provided it (flag bit 0) */
	if (mb_info->flags & 0x1) {
		terminal_writestring("Lower memory: ");
		terminal_writeuint(mb_info->mem_lower);
		terminal_writestring(" KB\n");

		terminal_writestring("Upper memory: ");
		terminal_writeuint(mb_info->mem_upper);
		terminal_writestring(" KB\n");
	}

	terminal_writestring("\nHello, kernel world!\n");
}
