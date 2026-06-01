#include "../include/serial.h"

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void serial_init(void)
{
    outb(COM1 + 1, 0x00); /* disable interrupts */
    outb(COM1 + 3, 0x80); /* enable DLAB to set baud rate */
    outb(COM1 + 0, 0x03); /* baud divisor low byte (38400 baud) */
    outb(COM1 + 1, 0x00); /* baud divisor high byte */
    outb(COM1 + 3, 0x03); /* 8 bits, no parity, one stop bit */
    outb(COM1 + 2, 0xC7); /* enable and clear FIFO */
    outb(COM1 + 4, 0x0B); /* enable RTS/DSR */
}

void serial_putchar(char c)
{
    /* wait until transmit buffer is empty */
    while (!(inb(COM1 + 5) & 0x20));
    outb(COM1, c);
}

void serial_write(const char* str)
{
    while (*str)
        serial_putchar(*str++);
}
