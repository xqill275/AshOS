#include "../include/keyboard.h"
#include "../include/shell.h"
#include <stdint.h>

#define KEYBOARD_DATA_PORT 0x60

/* normal scancode table */
static const char scancode_table[58] = {
    0,    0,   '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o',  'p',  '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g',  'h',  'j', 'k', 'l',  ';',
    '\'', '`', 0,  '\\','z',  'x',  'c', 'v', 'b',  'n',
    'm', ',', '.', '/',  0,    '*',  0,   ' '
};

/* shifted scancode table */
static const char scancode_table_shifted[58] = {
    0,    0,   '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R',
    'T', 'Y', 'U', 'I', 'O',  'P',  '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G',  'H',  'J', 'K', 'L',  ':',
    '"', '~', 0,   '|', 'Z',  'X',  'C', 'V', 'B',  'N',
    'M', '<', '>', '?',  0,    '*',  0,   ' '
};

/* modifier state */
static uint8_t shift_held  = 0;
static uint8_t caps_lock   = 0;

/* scancodes for modifier keys */
#define SCANCODE_LSHIFT       0x2A
#define SCANCODE_RSHIFT       0x36
#define SCANCODE_LSHIFT_REL   0xAA
#define SCANCODE_RSHIFT_REL   0xB6
#define SCANCODE_CAPS_LOCK    0x3A

static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void keyboard_handler(void)
{
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* handle shift press and release */
    if (scancode == SCANCODE_LSHIFT || scancode == SCANCODE_RSHIFT) {
        shift_held = 1;
        return;
    }
    if (scancode == SCANCODE_LSHIFT_REL || scancode == SCANCODE_RSHIFT_REL) {
        shift_held = 0;
        return;
    }

    /* handle caps lock toggle */
    if (scancode == SCANCODE_CAPS_LOCK) {
        caps_lock = !caps_lock;
        return;
    }

    /* ignore other key releases */
    if (scancode & 0x80)
        return;

    if (scancode < 58) {
        /* determine if we should use shifted table */
        uint8_t use_shift = shift_held ^ caps_lock;
        char c = use_shift ? scancode_table_shifted[scancode]
                           : scancode_table[scancode];
        if (c != 0)
            shell_process_char(c);
    }
}

void keyboard_init(void)
{
}
