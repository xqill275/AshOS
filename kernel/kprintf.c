#include "../include/kprintf.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

extern void terminal_putchar(char c);
extern void terminal_writestring(const char* str);

static void print_uint(uint32_t n, uint32_t base)
{
    static const char digits[] = "0123456789abcdef";
    char buf[32];
    int i = 0;

    if (n == 0) {
        terminal_putchar('0');
        return;
    }

    while (n > 0) {
        buf[i++] = digits[n % base];
        n /= base;
    }

    /* digits are in reverse order */
    while (i-- > 0)
        terminal_putchar(buf[i]);
}

static void print_int(int32_t n)
{
    if (n < 0) {
        terminal_putchar('-');
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
            terminal_putchar(*fmt++);
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
                terminal_writestring("0x");
                print_uint(va_arg(args, uint32_t), 16);
                break;
            case 's': {
                const char* s = va_arg(args, const char*);
                terminal_writestring(s ? s : "(null)");
                break;
            }
            case 'c':
                terminal_putchar((char)va_arg(args, int));
                break;
            case '%':
                terminal_putchar('%');
                break;
            default:
                terminal_putchar('%');
                terminal_putchar(*fmt);
                break;
        }
        fmt++;
    }

    va_end(args);
}
