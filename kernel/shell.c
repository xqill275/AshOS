#include "../include/shell.h"
#include "../include/kprintf.h"
#include <stdint.h>
#include <stddef.h>

extern void terminal_putchar(char c);
extern void terminal_writestring(const char* str);
extern void terminal_setcolor(uint8_t color);
extern void terminal_initialize(void);

#define VERSION "AshOS V200526"

static char buffer[SHELL_BUFFER_SIZE];
static size_t buffer_pos = 0;

static void shell_prompt(void)
{
    terminal_setcolor(0x0A); /* light green */
    kprintf("AshOS> ");
    terminal_setcolor(0x07); /* light grey */
}

static int strcmp(const char* a, const char* b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static int strncmp(const char* a, const char* b, size_t n)
{
    while (n-- && *a && *b && *a == *b) { a++; b++; }
    if (n == (size_t)-1) return 0;
    return *a - *b;
}

/* ============================================================
   Commands
   ============================================================ */

static void cmd_help(void)
{
    terminal_setcolor(0x0B); /* light cyan */
    kprintf("\nAvailable commands:\n");
    terminal_setcolor(0x07);
    kprintf("  help     - show this message\n");
    kprintf("  clear    - clear the screen\n");
    kprintf("  version  - show OS version\n");
    kprintf("  echo     - print text back\n");
}

static void cmd_clear(void)
{
    terminal_initialize();
}

static void cmd_version(void)
{
    terminal_setcolor(0x0B);
    kprintf("\n%s\n", VERSION);
    terminal_setcolor(0x07);
    kprintf("Built from scratch.\n");
}

static void cmd_echo(const char* args)
{
    kprintf("\n%s\n", args);
}

/* ============================================================
   Command processing
   ============================================================ */

static void shell_execute(void)
{
    kprintf("\n");

    if (buffer_pos == 0) {
        shell_prompt();
        return;
    }

    if (strcmp(buffer, "help") == 0) {
        cmd_help();
    } else if (strcmp(buffer, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(buffer, "version") == 0) {
        cmd_version();
    } else if (strncmp(buffer, "echo ", 5) == 0) {
        cmd_echo(buffer + 5);
    } else {
        terminal_setcolor(0x04); /* red */
        kprintf("Unknown command: %s\n", buffer);
        terminal_setcolor(0x07);
    }

    buffer_pos = 0;
    buffer[0]  = 0;
    terminal_putchar('\n');
    shell_prompt();
}

void shell_process_char(char c)
{
    if (c == '\n') {
        shell_execute();
        return;
    }

    /* handle backspace */
    if (c == '\b') {
        if (buffer_pos > 0) {
            buffer_pos--;
            buffer[buffer_pos] = 0;
            terminal_putchar('\b');
        }
        return;
    }

    /* ignore if buffer full */
    if (buffer_pos >= SHELL_BUFFER_SIZE - 1)
        return;

    buffer[buffer_pos++] = c;
    buffer[buffer_pos]   = 0;
    terminal_putchar(c);
}

void shell_init(void)
{
    buffer_pos = 0;
    buffer[0]  = 0;
    terminal_putchar('\n');
    shell_prompt();
}
