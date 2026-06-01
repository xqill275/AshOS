#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/keyboard.h"
#include "../include/shell.h"
#include "../include/kprintf.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/heap.h"
#include "../include/process.h"
#include "../include/ata.h"
#include "../include/fs.h"
#include "../include/syscall.h"
#include "../include/tss.h"

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

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
} multiboot_info_t;

/* ============================================================
   VGA text mode
   ============================================================ */

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t*)0xB8000)

enum vga_color {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
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
   Terminal
   ============================================================ */

static size_t    terminal_row;
static size_t    terminal_column;
static uint8_t   terminal_color;
static uint16_t* terminal_buffer;

#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void terminal_update_cursor(void)
{
    uint16_t pos = terminal_row * VGA_WIDTH + terminal_column;
    outb(VGA_CTRL_PORT, 0x0F);
    outb(VGA_DATA_PORT, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_PORT, 0x0E);
    outb(VGA_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_initialize(void)
{
    terminal_row    = 0;
    terminal_column = 0;
    terminal_color  = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;

    for (size_t y = 0; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            terminal_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
    terminal_update_cursor();
}

void terminal_setcolor(uint8_t color)
{
    terminal_color = color;
}

static void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    terminal_buffer[y * VGA_WIDTH + x] = vga_entry(c, color);
}

static void terminal_scroll(void)
{
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            terminal_buffer[y * VGA_WIDTH + x] =
                terminal_buffer[(y + 1) * VGA_WIDTH + x];

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
    if (c == '\r') { terminal_column = 0; return; }
    if (c == '\t') {
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
    terminal_update_cursor();
}

void terminal_write(const char* data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

/* ============================================================
   Minimal stdlib
   ============================================================ */

size_t strlen(const char* str)
{
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

void terminal_writestring(const char* data)
{
    terminal_write(data, strlen(data));
}

void terminal_writeuint(uint32_t n)
{
    if (n == 0) { terminal_putchar('0'); return; }
    char buf[10];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i-- > 0) terminal_putchar(buf[i]);
}



/* ============================================================
   Kernel entry
   ============================================================ */

void kernel_main(uint32_t magic, multiboot_info_t* mb_info)
{
    gdt_init();
    terminal_initialize();
    idt_init();
    pmm_init(mb_info->mem_upper);
    vmm_init();
    heap_init();
    process_init();
    ata_init();
    fs_init();
    syscall_init();
    tss_init(0x200000); /* kernel stack at 2MB */
    keyboard_init();
    //__asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xFE), "Nd"((uint16_t)0x21));

    if (magic != MULTIBOOT_MAGIC) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        kprintf("ERROR: Not loaded by a multiboot bootloader!\n");
        return;
    }

    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    kprintf("=== AshOS ===\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    kprintf("Booted successfully via GRUB.\n\n");

    kprintf("Lower memory: %d KB\n", mb_info->mem_lower);
    kprintf("Upper memory: %d KB\n", mb_info->mem_upper);

    kprintf("\nHello, kernel world!\n");

    shell_init();

    /* keep kernel alive for interrupts */
    while (1) __asm__ volatile ("hlt");
}
