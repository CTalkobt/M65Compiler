/* sscanf.c — Formatted string input for cc45 / MEGA65
 *
 * int sscanf(char *str, char *fmt, ...);
 *
 * Supported format specifiers:
 *   %d   — signed decimal integer (16-bit int *)
 *   %u   — unsigned decimal integer (16-bit unsigned int *)
 *   %x   — hexadecimal integer (16-bit int *)
 *   %o   — octal integer (16-bit int *)
 *   %c   — single character (char *)
 *   %s   — whitespace-delimited string (char *)
 *   %ld  — signed decimal long (32-bit long *)
 *   %lu  — unsigned decimal long (32-bit unsigned long *)
 *   %lx  — hexadecimal long (32-bit long *)
 *   %lo  — octal long (32-bit long *)
 *   %%   — literal '%' (matched, not assigned)
 *
 * Returns the number of input items successfully matched and assigned,
 * or 0 if no conversions succeeded.
 *
 * Note: Format specifiers are matched in PETSCII encoding (case-swapped
 * relative to ASCII) because string literals go through .text conversion.
 */

#include <stdarg.h>

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    /* PETSCII: lowercase a-f are $41-$46, uppercase A-F are $61-$66 */
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;  /* PETSCII lowercase */
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;  /* ASCII lowercase */
    return -1;
}

int sscanf(char *str, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char *inp = str;
    int matched = 0;

    while (*fmt) {
        /* Whitespace in format: skip any whitespace in input */
        if (is_space(*fmt)) {
            while (is_space(*inp)) inp = inp + 1;
            fmt = fmt + 1;
            continue;
        }

        /* Literal match (non-%) */
        if (*fmt != '%') {
            if (*inp != *fmt) break;
            inp = inp + 1;
            fmt = fmt + 1;
            continue;
        }
        fmt = fmt + 1; /* skip '%' */

        /* %% — literal percent */
        if (*fmt == '%') {
            if (*inp != '%') break;
            inp = inp + 1;
            fmt = fmt + 1;
            continue;
        }

        /* Check for 'l' length modifier */
        int isLong = 0;
        if (*fmt == 'L' || *fmt == 'l') {
            isLong = 1;
            fmt = fmt + 1;
        }

        /* %d / %ld — signed decimal */
        if (*fmt == 'D' || *fmt == 'd') {
            /* Skip leading whitespace */
            while (is_space(*inp)) inp = inp + 1;
            if (*inp == 0) break;

            int neg = 0;
            if (*inp == '-') { neg = 1; inp = inp + 1; }
            else if (*inp == '+') { inp = inp + 1; }

            if (*inp < '0' || *inp > '9') break;

            if (isLong) {
                long val = 0;
                while (*inp >= '0' && *inp <= '9') {
                    val = val * 10 + (*inp - '0');
                    inp = inp + 1;
                }
                if (neg) val = -val;
                long *dest = (long *)va_arg(ap, int);
                *dest = val;
            } else {
                int val = 0;
                while (*inp >= '0' && *inp <= '9') {
                    val = val * 10 + (*inp - '0');
                    inp = inp + 1;
                }
                if (neg) val = -val;
                int *dest = (int *)va_arg(ap, int);
                *dest = val;
            }
            matched = matched + 1;

        /* %u / %lu — unsigned decimal */
        } else if (*fmt == 'U' || *fmt == 'u') {
            while (is_space(*inp)) inp = inp + 1;
            if (*inp < '0' || *inp > '9') break;

            if (isLong) {
                unsigned long val = 0;
                while (*inp >= '0' && *inp <= '9') {
                    val = val * 10 + (*inp - '0');
                    inp = inp + 1;
                }
                long *dest = (long *)va_arg(ap, int);
                *dest = (long)val;
            } else {
                unsigned int val = 0;
                while (*inp >= '0' && *inp <= '9') {
                    val = val * 10 + (*inp - '0');
                    inp = inp + 1;
                }
                int *dest = (int *)va_arg(ap, int);
                *dest = (int)val;
            }
            matched = matched + 1;

        /* %x / %lx — hexadecimal */
        } else if (*fmt == 'X' || *fmt == 'x') {
            while (is_space(*inp)) inp = inp + 1;
            /* Skip optional 0x / 0X / $ prefix */
            if (*inp == '$') {
                inp = inp + 1;
            } else if (*inp == '0' && (*(inp+1) == 'X' || *(inp+1) == 'x'
                                    || *(inp+1) == 'X'+0x20 || *(inp+1) == 'x'-0x20)) {
                inp = inp + 2;
            }
            int d = hex_digit(*inp);
            if (d < 0) break;

            if (isLong) {
                long val = 0;
                while ((d = hex_digit(*inp)) >= 0) {
                    val = (val << 4) | d;
                    inp = inp + 1;
                }
                long *dest = (long *)va_arg(ap, int);
                *dest = val;
            } else {
                int val = 0;
                while ((d = hex_digit(*inp)) >= 0) {
                    val = (val << 4) | d;
                    inp = inp + 1;
                }
                int *dest = (int *)va_arg(ap, int);
                *dest = val;
            }
            matched = matched + 1;

        /* %o / %lo — octal */
        } else if (*fmt == 'O' || *fmt == 'o') {
            while (is_space(*inp)) inp = inp + 1;
            if (*inp < '0' || *inp > '7') break;

            if (isLong) {
                long val = 0;
                while (*inp >= '0' && *inp <= '7') {
                    val = (val << 3) | (*inp - '0');
                    inp = inp + 1;
                }
                long *dest = (long *)va_arg(ap, int);
                *dest = val;
            } else {
                int val = 0;
                while (*inp >= '0' && *inp <= '7') {
                    val = (val << 3) | (*inp - '0');
                    inp = inp + 1;
                }
                int *dest = (int *)va_arg(ap, int);
                *dest = val;
            }
            matched = matched + 1;

        /* %c — single character */
        } else if (*fmt == 'C' || *fmt == 'c') {
            if (*inp == 0) break;
            char *dest = (char *)va_arg(ap, int);
            *dest = *inp;
            inp = inp + 1;
            matched = matched + 1;

        /* %s — whitespace-delimited string */
        } else if (*fmt == 'S' || *fmt == 's') {
            while (is_space(*inp)) inp = inp + 1;
            if (*inp == 0) break;
            char *dest = (char *)va_arg(ap, int);
            while (*inp && !is_space(*inp)) {
                *dest = *inp;
                dest = dest + 1;
                inp = inp + 1;
            }
            *dest = 0;
            matched = matched + 1;
        } else {
            /* Unknown specifier — stop */
            break;
        }

        fmt = fmt + 1;
    }

    va_end(ap);
    return matched;
}
