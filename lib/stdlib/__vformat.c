/* __vformat.c — Formatting core of the printf family for cc45 / MEGA65
 *
 * int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt,
 *               va_list ap, int (*ffmt)(char *buf, float val,
 *                                       int conv, int prec, int alt));
 *
 * Writes each output character through put() and returns the count.
 * %[flags][width][.precision][length]conversion with
 *   flags      - + space # 0
 *   width      digits or *
 *   precision  .digits or .*
 *   length     hh h l ll L j z t
 *   conversion d i u x X o b p c s n % and f F e E g G (through ffmt; a
 *              program that prints no floats passes 0 and they are skipped)
 * int is 16 bits and long 32; long long (8 bytes) prints its low 32 bits.
 * %p prints $hhhh.
 */

#include <stdarg.h>

typedef void (*__fmt_put)(int c, void *ctx);

static void vf_rep(__fmt_put put, void *ctx, int c, int n) {
    while (n > 0) { put(c, ctx); n = n - 1; }
}

int __vformat(__fmt_put put, void *ctx, char *fmt, va_list ap, int (*ffmt)(char *, float, int, int, int)) {
    int count = 0;
    char buf[64];   /* a float with %f: up to 39 digits, '.', 20 decimals */

    while (*fmt) {
        int c = *fmt;
        fmt = fmt + 1;
        if (c != '%') { put(c, ctx); count = count + 1; continue; }

        /* Flags */
        int left = 0, plus = 0, space = 0, alt = 0, zero = 0;
        for (;;) {
            c = *fmt;
            if (c == '-') left = 1;
            else if (c == '+') plus = 1;
            else if (c == ' ') space = 1;
            else if (c == '#') alt = 1;
            else if (c == '0') zero = 1;
            else break;
            fmt = fmt + 1;
        }

        /* Width */
        int width = 0;
        if (*fmt == '*') {
            width = va_arg(ap, int);
            if (width < 0) { left = 1; width = -width; }
            fmt = fmt + 1;
        } else {
            while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt - '0'); fmt = fmt + 1; }
        }

        /* Precision */
        int prec = -1;
        if (*fmt == '.') {
            fmt = fmt + 1;
            prec = 0;
            if (*fmt == '*') {
                prec = va_arg(ap, int);
                if (prec < 0) prec = -1;
                fmt = fmt + 1;
            } else {
                while (*fmt >= '0' && *fmt <= '9') { prec = prec * 10 + (*fmt - '0'); fmt = fmt + 1; }
            }
        }

        /* Length: the argument's size in bytes */
        int size = 2;
        if (*fmt == 'h') {
            fmt = fmt + 1;
            if (*fmt == 'h') { size = 1; fmt = fmt + 1; }
        } else if (*fmt == 'l' || *fmt == 'L' || *fmt == 'j') {
            size = 4;
            fmt = fmt + 1;
            if (*fmt == 'l') { size = 8; fmt = fmt + 1; }
        } else if (*fmt == 'z' || *fmt == 't') {
            fmt = fmt + 1;
        }

        c = *fmt;
        if (c) fmt = fmt + 1;

        char *s = buf;      /* the converted text */
        int len = 0;        /* its length */
        char *pfx = "";     /* sign or base prefix */
        int pfxLen = 0;
        int zeros = 0;      /* zeros between the prefix and the digits */

        if (c == 'd' || c == 'i' || c == 'u' || c == 'x' || c == 'X' ||
            c == 'o' || c == 'b' || c == 'p') {
            unsigned long v;
            int neg = 0;
            if (c == 'p') {
                v = (unsigned int)va_arg(ap, int);
                pfx = "$"; pfxLen = 1;
                if (prec < 0) prec = 4;
            } else if (size >= 4) {
                v = (unsigned long)va_arg(ap, long);
                if (size == 8) va_arg(ap, long);
                if ((c == 'd' || c == 'i') && (long)v < 0) { neg = 1; v = 0 - v; }
            } else {
                unsigned int w = (unsigned int)va_arg(ap, int);
                if (size == 1) {
                    w = w & 0xFF;
                    if ((c == 'd' || c == 'i') && w >= 0x80) { neg = 1; w = 0x100 - w; }
                } else if ((c == 'd' || c == 'i') && w >= 0x8000) {
                    neg = 1; w = 0 - w;
                }
                v = w;
            }
            unsigned int base = 10;
            if (c == 'x' || c == 'X' || c == 'p') base = 16;
            else if (c == 'o') base = 8;
            else if (c == 'b') base = 2;
            char *digits = (c == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";

            /* Digits, from the end of the buffer */
            s = buf + sizeof(buf);
            if (v || prec != 0) {
                do {
                    s = s - 1;
                    *s = digits[(int)(v % base)];
                    v = v / base;
                    len = len + 1;
                } while (v);
            }
            if (prec >= 0) { if (prec > len) zeros = prec - len; }
            if (c == 'd' || c == 'i') {
                if (neg) { pfx = "-"; pfxLen = 1; }
                else if (plus) { pfx = "+"; pfxLen = 1; }
                else if (space) { pfx = " "; pfxLen = 1; }
            } else if (alt) {
                if (c == 'o') {
                    /* The first digit is a 0 */
                    if (zeros == 0 && (len == 0 || *s != '0')) zeros = 1;
                } else if (len && *s != '0' && (c == 'x' || c == 'X' || c == 'b')) {
                    pfx = (c == 'x') ? "0x" : (c == 'X') ? "0X" : "0b";
                    pfxLen = 2;
                }
            }
            /* The 0 flag pads with zeros when there is no precision */
            if (zero && !left && prec < 0 && width > pfxLen + len)
                zeros = width - pfxLen - len;
        } else if (c == 'c') {
            buf[0] = (char)va_arg(ap, int);
            len = 1;
        } else if (c == 's') {
            s = (char *)va_arg(ap, int);
            if (!s) s = "(null)";
            while (s[len] && (prec < 0 || len < prec)) len = len + 1;
        } else if (c == 'n') {
            char *p = (char *)va_arg(ap, int);
            p[0] = (char)count;
            if (size >= 2) p[1] = (char)(count >> 8);
            if (size >= 4) { p[2] = (count < 0) ? 0xFF : 0; p[3] = p[2]; }
            continue;
        } else if (c == 'f' || c == 'F' || c == 'e' || c == 'E' || c == 'g' || c == 'G') {
            float fv = va_arg(ap, float);
            if (!ffmt) continue;
            /* the conversion, precision and # flag are ffmt's (it rounds) */
            len = ffmt(buf, fv, c, prec, alt);
            s = buf;
            if (*s == '-') { pfx = "-"; pfxLen = 1; s = s + 1; len = len - 1; }
            else if (plus) { pfx = "+"; pfxLen = 1; }
            else if (space) { pfx = " "; pfxLen = 1; }
            if (zero && !left && width > pfxLen + len) zeros = width - pfxLen - len;
        } else if (c == '%') {
            buf[0] = '%';
            len = 1;
        } else {
            /* Unknown conversion: print it as is */
            if (!c) break;
            buf[0] = (char)c;
            len = 1;
        }

        int total = pfxLen + zeros + len;
        if (!left) vf_rep(put, ctx, ' ', width - total);
        int i;
        for (i = 0; i < pfxLen; i++) put(pfx[i], ctx);
        vf_rep(put, ctx, '0', zeros);
        for (i = 0; i < len; i++) put(s[i], ctx);
        if (left) vf_rep(put, ctx, ' ', width - total);
        count = count + (total > width ? total : width);
    }
    return count;
}
