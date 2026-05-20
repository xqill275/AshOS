#include "../include/keyboard.h"
#include "../include/shell.h"
#include <stdint.h>

extern void terminal_putchar(char c);

#define KEYBOARD_DATA_PORT 0x60

static const char scancode_table[58] = {
    0,    0,   '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o',  'p',  '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g',  'h',  'j', 'k', 'l',  ';',
    '\'', '`', 0,  '\\','z',  'x',  'c', 'v', 'b',  'n',
    'm', ',', '.', '/',  0,    '*',  0,   ' '
};

static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void keyboard_handler(void)
{
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode & 0x80)
        return;

    if (scancode < 58) {
        char c = scancode_table[scancode];
        if (c != 0)
            shell_process_char(c);
    }
}

void keyboard_init(void)
{
}
