/* out45.c — the suite's out.h for cc45's harness (#129): print, print_int
 * and print_hex keep the text at $C100 (up to 3 KB, NUL-terminated), where
 * run_bench.py reads the benchmark's results from xemu's memory dump. Not
 * on the screen: a screen that scrolls keeps xemu's -prgexit from seeing
 * READY. (main45.c prints one line.)
 */
#include "out.h"

#define LOG ((volatile char *)0xC100)
#define LOG_MAX 0x0BFF
static unsigned int logLen;

static void out(char c) {
    if (logLen < LOG_MAX) {
        LOG[logLen++] = c;
        LOG[logLen] = 0;
    }
}

void print(const char *s) {
    while (*s) out(*s++);
}

static void digits(unsigned long n, unsigned char base, int padLength) {
    char buffer[12];
    int i = 0;
    do {
        unsigned char d = (unsigned char)(n % base);
        buffer[i++] = d < 10 ? '0' + d : 'A' + d - 10;
        n /= base;
    } while (n);
    while (i < padLength && i < 11) buffer[i++] = '0';
    while (i > 0) out(buffer[--i]);
}

void print_int(long n, int padLength) {
    if (n < 0) {
        out('-');
        n = -n;
    }
    digits((unsigned long)n, 10, padLength);
}

void print_hex(unsigned long n, int padLength) {
    digits(n, 16, padLength);
}
