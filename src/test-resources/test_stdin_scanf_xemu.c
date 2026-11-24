/* test_stdin_scanf_xemu.c — the scanf family on stdin (the keyboard, a
 * line typed into the KERNAL's buffer) and through a va_list; fflush;
 * putc and getc
 *
 * Results at $C0C0 (01 = pass):
 *   [0] scanf("%d %d") of a typed line
 *   [1] vscanf: the same through a va_list
 *   [2] vsscanf: a string through a va_list; its count, a matching failure
 *   [3] fflush(stdout) and fflush(NULL): 0
 *   [4] putc / getc (#750): the macros, the functions by address and after
 *       #undef
 *   [5] $AA sentinel
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define KEYBUF ((volatile unsigned char *)0x02B0)
#define KEYCOUNT (*(volatile unsigned char *)0x00D0)

static void keys(const char *k) {
    unsigned char n = 0;
    while (k[n]) { KEYBUF[n] = k[n]; n++; }
    KEYCOUNT = n;
}

static int my_scanf(const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vscanf(fmt, ap);
    va_end(ap);
    return n;
}

static int my_sscanf(const char *s, const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsscanf(s, fmt, ap);
    va_end(ap);
    return n;
}

/* (taken before the #undef: a function-like macro's name alone is the
 * function) */
static int (*const put_fn)(int, FILE *) = putc;
static int (*const get_fn)(FILE *) = getc;

static int t_putc_getc(void) {
    int ok;
    ok = putc('a', stdout) == 'a' && put_fn('b', stdout) == 'b';
    /* the '\n' vscanf left (it stops before it), then a new line */
    ok = ok && getc(stdin) == '\n';
    keys("xyz\n");
    ok = ok && getc(stdin) == 'x' && get_fn(stdin) == 'y';
#undef putc
#undef getc
    ok = ok && putc('c', stdout) == 'c' && getc(stdin) == 'z' && getc(stdin) == '\n';
    return ok && put_fn == putc && get_fn == getc;
}

int main(void) {
    int a = 0, b = 0, n;
    char w[8];
    OUT[5] = 0;

    keys("12 34\n");               /* ('\n' is RETURN, $0D) */
    n = scanf("%d %d", &a, &b);
    OUT[0] = n == 2 && a == 12 && b == 34;

    a = b = 0;
    keys("56 78\n");
    n = my_scanf("%d %d", &a, &b);
    OUT[1] = n == 2 && a == 56 && b == 78;

    a = b = 0;
    n = my_sscanf("x=9 word", "x=%d %7s", &a, w);
    OUT[2] = n == 2 && a == 9 && strcmp(w, "word") == 0 && my_sscanf("abc", "%d", &b) == 0 &&
             my_sscanf("", "%d", &b) == EOF;

    printf("ok");
    OUT[3] = fflush(stdout) == 0 && fflush(NULL) == 0;
    OUT[4] = t_putc_getc();
    OUT[5] = 0xAA;
    return 0;
}
