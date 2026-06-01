#include "../include/kprintf.h"
#include "../include/serial.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

extern void terminal_putchar(char c);
extern void terminal_writestring(const char* str);

/* write a single character to both terminal and serial */
static void put(char c)
{
    terminal_putchar(c);
    serial_putchar(c);
}

/* write a string to both terminal and serial */
static void puts(const char* s)
{
    while (*s) {
        terminal_putchar(*s);
        serial_putchar(*s);
        s++;
    }
}

static void print_uint(uint32_t n, uint32_t base)
{
    static const char digits[] = "0123456789abcdef";
    char buf[32];
    int i = 0;

    if (n == 0) {
        put('0');
        return;
    }

    while (n > 0) {
        buf[i++] = digits[n % base];
        n /= base;
    }

    while (i-- > 0)
        put(buf[i]);
}

static void print_int(int32_t n)
{
    if (n < 0) {
        put('-');
        print_uint((uint32_t)(-n), 10);
    } else {
        print_uint((uint32_t)n, 10);
    }
}

void kprintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            put(*fmt++);
            continue;
        }

        fmt++; /* skip the % */

        switch (*fmt) {
            case 'd':
                print_int(va_arg(args, int32_t));
                break;
            case 'u':
                print_uint(va_arg(args, uint32_t), 10);
                break;
            case 'x':
                puts("0x");
                print_uint(va_arg(args, uint32_t), 16);
                break;
            case 's': {
                const char* s = va_arg(args, const char*);
                puts(s ? s : "(null)");
                break;
            }
            case 'c':
                put((char)va_arg(args, int));
                break;
            case '%':
                put('%');
                break;
            default:
                put('%');
                put(*fmt);
                break;
        }
        fmt++;
    }

    va_end(args);
}
