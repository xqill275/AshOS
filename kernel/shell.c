#include "../include/shell.h"
#include "../include/kprintf.h"
#include "../include/pmm.h"
#include "../include/process.h"
#include "../include/fs.h"
#include "../include/heap.h"
#include "../include/elf.h"
#include "../include/usermode.h"

#include <stdint.h>
#include <stddef.h>

extern void terminal_putchar(char c);
extern void terminal_writestring(const char* str);
extern void terminal_setcolor(uint8_t color);
extern void terminal_initialize(void);

#define VERSION "AshOS V210526"

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
    kprintf("  meminfo  - show memory usage\n");
    kprintf("  ps       - list processes\n");
    kprintf("  ls            - list files\n");
    kprintf("  touch <file>  - create a file\n");
    kprintf("  write <file>  - write to a file\n");
    kprintf("  cat <file>    - read a file\n");
    kprintf("  rm <file>     - delete a file\n");
    kprintf("  run <file>    - run an ELF program\n");
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

static void cmd_meminfo(void)
{
    kprintf("\nTotal pages: %d\n", pmm_free_pages() + 521);
    kprintf("Free pages:  %d\n", pmm_free_pages());
    kprintf("Free memory: %d KB\n", pmm_free_pages() * 4);
}

static void cmd_ps(void)
{
    kprintf("\nPID\tSTATE\n");
    kprintf("---\t-----\n");

    process_t* proc = process_list;
    while (proc) {
        const char* state;
        switch (proc->state) {
            case PROCESS_RUNNING: state = "RUNNING"; break;
            case PROCESS_READY:   state = "READY";   break;
            case PROCESS_DEAD:    state = "DEAD";    break;
            default:              state = "UNKNOWN"; break;
        }
        kprintf("%d\t%s\n", proc->pid, state);
        proc = proc->next;
    }
}

static void cmd_ls(void)
{
    fs_list();
}

static void cmd_touch(const char* args)
{
    if (args[0] == 0) {
        kprintf("Usage: touch <filename>\n");
        return;
    }
    fs_create(args);
}

static void cmd_write(const char* args)
{
    const char* space = args;
    while (*space && *space != ' ') space++;
    if (*space == 0) {
        kprintf("Usage: write <filename> <content>\n");
        return;
    }

    char name[64];
    int len = space - args;
    for (int i = 0; i < len; i++) name[i] = args[i];
    name[len] = 0;

    const char* content = space + 1;

    /* calculate content length */
    uint32_t content_len = 0;
    while (content[content_len]) content_len++;

    fs_write(name, (const uint8_t*)content, content_len);
}

static void cmd_cat(const char* args)
{
    if (args[0] == 0) {
        kprintf("Usage: cat <filename>\n");
        return;
    }

    uint8_t* buf = (uint8_t*)kmalloc(512);
    int bytes = fs_read(args, buf, 512);
    if (bytes > 0) {
        buf[bytes] = 0;
        kprintf("\n%s\n", (char*)buf);
    }
    kfree(buf);
}

static void cmd_rm(const char* args)
{
    if (args[0] == 0) {
        kprintf("Usage: rm <filename>\n");
        return;
    }
    fs_delete(args);
}

static void cmd_run(const char* args)
{
    if (args[0] == 0) {
        kprintf("Usage: run <filename>\n");
        return;
    }

    uint32_t entry = 0;
    if (elf_load(args, &entry) < 0) {
        kprintf("Failed to load '%s'\n", args);
        return;
    }

    /* allocate user stack */
    uint32_t user_stack = (uint32_t)kmalloc(4096) + 4096;
    enter_usermode(entry, user_stack);
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
    } else if (strcmp(buffer, "meminfo") == 0) {
        cmd_meminfo();
    } else if (strcmp(buffer, "ps") == 0) {
        cmd_ps(); 
    } else if (strcmp(buffer, "ls") == 0) {
        cmd_ls();
    } else if (strncmp(buffer, "touch ", 6) == 0) {
        cmd_touch(buffer + 6);
    } else if (strncmp(buffer, "write ", 6) == 0) {
        cmd_write(buffer + 6);
    } else if (strncmp(buffer, "cat ", 4) == 0) {
        cmd_cat(buffer + 4);
    } else if (strncmp(buffer, "rm ", 3) == 0) {
        cmd_rm(buffer + 3);
    } else if (strncmp(buffer, "run ", 4) == 0) {
    cmd_run(buffer + 4);
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
