/* sscanf.c — Formatted string input for cc45 / MEGA65
 *
 * int sscanf(char *str, char *fmt, ...);
 *
 * Supports: %d %u %x %o %c %s %% and long variants %ld %lu %lx %lo.
 *
 * Returns the number of input items successfully matched and assigned.
 */

#include <stdarg.h>

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static char *scan_int(char *inp, int *result) {
    while (is_space(*inp)) inp = inp + 1;
    int neg = 0;
    if (*inp == '-') { neg = 1; inp = inp + 1; }
    else if (*inp == '+') { inp = inp + 1; }
    if (*inp < '0' || *inp > '9') return 0;
    int val = 0;
    while (*inp >= '0' && *inp <= '9') {
        val = val * 10 + (*inp - '0');
        inp = inp + 1;
    }
    if (neg) val = -val;
    *result = val;
    return inp;
}

static char *scan_long(char *inp, long *result) {
    while (is_space(*inp)) inp = inp + 1;
    int neg = 0;
    if (*inp == '-') { neg = 1; inp = inp + 1; }
    else if (*inp == '+') { inp = inp + 1; }
    if (*inp < '0' || *inp > '9') return 0;
    long val = 0;
    while (*inp >= '0' && *inp <= '9') {
        val = val * 10 + (*inp - '0');
        inp = inp + 1;
    }
    if (neg) val = -val;
    *result = val;
    return inp;
}

static char *scan_hex(char *inp, int *result) {
    while (is_space(*inp)) inp = inp + 1;
    if (*inp == '$') inp = inp + 1;
    else if (*inp == '0' && (*(inp+1) == 'x' || *(inp+1) == 'X')) inp = inp + 2;
    int d = hex_digit(*inp);
    if (d < 0) return 0;
    int val = 0;
    while ((d = hex_digit(*inp)) >= 0) {
        val = (val << 4) | d;
        inp = inp + 1;
    }
    *result = val;
    return inp;
}

static char *scan_hex_long(char *inp, long *result) {
    while (is_space(*inp)) inp = inp + 1;
    if (*inp == '$') inp = inp + 1;
    else if (*inp == '0' && (*(inp+1) == 'x' || *(inp+1) == 'X')) inp = inp + 2;
    int d = hex_digit(*inp);
    if (d < 0) return 0;
    long val = 0;
    while ((d = hex_digit(*inp)) >= 0) {
        val = (val << 4) | d;
        inp = inp + 1;
    }
    *result = val;
    return inp;
}

static char *scan_oct(char *inp, int *result) {
    while (is_space(*inp)) inp = inp + 1;
    if (*inp < '0' || *inp > '7') return 0;
    int val = 0;
    while (*inp >= '0' && *inp <= '7') {
        val = (val << 3) | (*inp - '0');
        inp = inp + 1;
    }
    *result = val;
    return inp;
}

static char *scan_oct_long(char *inp, long *result) {
    while (is_space(*inp)) inp = inp + 1;
    if (*inp < '0' || *inp > '7') return 0;
    long val = 0;
    while (*inp >= '0' && *inp <= '7') {
        val = (val << 3) | (*inp - '0');
        inp = inp + 1;
    }
    *result = val;
    return inp;
}

int sscanf(char *str, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char *inp = str;
    int matched = 0;

    while (*fmt) {
        if (is_space(*fmt)) {
            while (is_space(*inp)) inp = inp + 1;
            fmt = fmt + 1;
            continue;
        }

        if (*fmt != '%') {
            if (*inp != *fmt) break;
            inp = inp + 1; fmt = fmt + 1;
            continue;
        }
        fmt = fmt + 1;

        if (*fmt == '%') {
            if (*inp != '%') break;
            inp = inp + 1; fmt = fmt + 1;
            continue;
        }

        /* Long specifiers: %ld %lu %lx %lo */
        if (*fmt == 'L' || *fmt == 'l') {
            fmt = fmt + 1;
            if (*fmt == 'D' || *fmt == 'd') {
                long *dest = (long *)va_arg(ap, int);
                inp = scan_long(inp, dest);
                if (!inp) break;
                matched = matched + 1;
            } else if (*fmt == 'U' || *fmt == 'u') {
                long *dest = (long *)va_arg(ap, int);
                inp = scan_long(inp, dest);
                if (!inp) break;
                matched = matched + 1;
            } else if (*fmt == 'X' || *fmt == 'x') {
                long *dest = (long *)va_arg(ap, int);
                inp = scan_hex_long(inp, dest);
                if (!inp) break;
                matched = matched + 1;
            } else if (*fmt == 'O' || *fmt == 'o') {
                long *dest = (long *)va_arg(ap, int);
                inp = scan_oct_long(inp, dest);
                if (!inp) break;
                matched = matched + 1;
            }
            fmt = fmt + 1;
            continue;
        }

        /* Standard specifiers */
        if (*fmt == 'D' || *fmt == 'd') {
            int *dest = (int *)va_arg(ap, int);
            inp = scan_int(inp, dest);
            if (!inp) break;
            matched = matched + 1;
        } else if (*fmt == 'U' || *fmt == 'u') {
            int *dest = (int *)va_arg(ap, int);
            inp = scan_int(inp, dest);
            if (!inp) break;
            matched = matched + 1;
        } else if (*fmt == 'X' || *fmt == 'x') {
            int *dest = (int *)va_arg(ap, int);
            inp = scan_hex(inp, dest);
            if (!inp) break;
            matched = matched + 1;
        } else if (*fmt == 'O' || *fmt == 'o') {
            int *dest = (int *)va_arg(ap, int);
            inp = scan_oct(inp, dest);
            if (!inp) break;
            matched = matched + 1;
        } else if (*fmt == 'C' || *fmt == 'c') {
            if (*inp == 0) break;
            char *dest = (char *)va_arg(ap, int);
            *dest = *inp;
            inp = inp + 1;
            matched = matched + 1;
        } else if (*fmt == 'S' || *fmt == 's') {
            while (is_space(*inp)) inp = inp + 1;
            if (*inp == 0) break;
            char *dest = (char *)va_arg(ap, int);
            while (*inp && !is_space(*inp)) {
                *dest = *inp; dest = dest + 1; inp = inp + 1;
            }
            *dest = 0;
            matched = matched + 1;
        } else {
            break;
        }
        fmt = fmt + 1;
    }

    va_end(ap);
    return matched;
}
