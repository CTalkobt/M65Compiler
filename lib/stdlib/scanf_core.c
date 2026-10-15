/* scanf_core.c — the scanf engine behind sscanf/vsscanf and fscanf/vfscanf
 *
 * Reads characters through a callback (a string or a FILE) with one
 * character of lookahead, which is pushed back at the end (ungetc for a
 * FILE).
 *
 * Conversions: %d %i %u %o %x %X %p %c %s %[set] %[^set] %n %%, and %f %e
 * %g (upper case too) with the float scanf (scanf_core.h); field width; '*' (assignment suppression); length
 * modifiers hh h l ll L j z t (ll stores 8 bytes, the upper 4 the sign).
 * %x also accepts CBM's '$' prefix. Letters are digits in any case and in
 * either PETSCII letter range ('A' in cc45 source is $61, shifted keys give
 * $C1-$DA).
 *
 * Returns the number of items assigned, or EOF when the input ended before
 * the first conversion.
 */

#pragma cc45 signed_default

#include "scanf_core.h"

static int sc_space(int c) {
    return c == ' ' || (c >= 9 && c <= 13);
}

/* Digit value of c (letters in any encoding), or 99 */
static int sc_digit(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 0x41 && c <= 0x5A) return c - 0x41 + 10;
    if (c >= 0x61 && c <= 0x7A) return c - 0x61 + 10;
    if (c >= 0xC1 && c <= 0xDA) return c - 0xC1 + 10;
    return 99;
}

static int sc_is_x(int c) {
    return c == 0x58 || c == 0x78 || c == 0xD8;
}

/* Lower-case a conversion letter of the format ('D' and 'd' are the same) */
static int sc_lower(int c) {
    if (c >= 0x61 && c <= 0x7A) return c - 0x20;
    if (c >= 0xC1 && c <= 0xDA) return c - 0x80;
    return c;
}

/* Store v in an integer of the given size (1, 2, 4 or 8 bytes) */
static void sc_store_int(void *p, int size, unsigned long v, int neg) {
    unsigned char *b = (unsigned char *)p;
    int i;
    if (size == 1) { *b = (unsigned char)v; return; }
    if (size == 2) { *(unsigned int *)p = (unsigned int)v; return; }
    *(unsigned long *)p = v;
    if (size == 8) for (i = 4; i < 8; i++) b[i] = neg ? 0xFF : 0;
}

int __vscanf_core(FILE *file, const char *str, const char *fmt, va_list ap) {
    struct __scan_src src;
    struct __scan_src *s = &src;
    const unsigned char *f = (const unsigned char *)fmt;
    int assigned = 0;
    int converted = 0;         /* conversions done (assigned or suppressed) */
    int input_failure = 0;
    src.file = file; src.str = (const unsigned char *)str; src.la = SC_NONE; src.count = 0;

    while (*f) {
        int c = *f;
        if (sc_space(c)) {
            while (sc_space(sc_peek(s))) sc_next(s);
            f++;
            continue;
        }
        if (c != '%' || f[1] == '%') {
            if (c == '%') {
                f++;
                while (sc_space(sc_peek(s))) sc_next(s);
            }
            if (sc_peek(s) == SC_EOF) { input_failure = 1; break; }
            if (sc_peek(s) != *f) break;
            sc_next(s);
            f++;
            continue;
        }
        f++;

        /* %[*][width][length]conversion */
        int suppress = 0;
        int width = 0;
        int size = 2;           /* integer: 2 (int); float: any */
        if (*f == '*') { suppress = 1; f++; }
        while (*f >= '0' && *f <= '9') { width = width * 10 + (*f - '0'); f++; }
        for (;;) {
            int m = *f;
            if (m == 'h') { size = (f[1] == 'h') ? 1 : 2; f += (f[1] == 'h') ? 2 : 1; }
            else if (m == 'l') { size = (f[1] == 'l') ? 8 : 4; f += (f[1] == 'l') ? 2 : 1; }
            else if (m == 'L' || m == 'j') { size = 4; f++; }
            else if (m == 'z' || m == 't') { size = 2; f++; }
            else break;
        }
        int conv = *f;
        if (!conv) break;
        f++;
        int lc = sc_lower(conv);

        if (conv == 'n') {
            if (!suppress) sc_store_int(va_arg(ap, void *), size, (unsigned long)s->count, 0);
            continue;
        }
        if (lc != 'c' && conv != '[') {
            while (sc_space(sc_peek(s))) sc_next(s);
        }
        if (sc_peek(s) == SC_EOF) { input_failure = 1; break; }
        if (width == 0) width = (lc == 'c') ? 1 : 0x7FFF;

        if (lc == 'd' || lc == 'i' || lc == 'u' || lc == 'o' || lc == 'x' || lc == 'p') {
            int base = lc == 'o' ? 8 : (lc == 'x' || lc == 'p') ? 16 : (lc == 'i') ? 0 : 10;
            int neg = 0, got = 0, d;
            unsigned long v = 0;
            int ch = sc_peek(s);
            if (width && (ch == '+' || ch == '-')) { neg = ch == '-'; sc_next(s); width--; }
            if (width && base == 16 && sc_peek(s) == '$') { sc_next(s); width--; }
            else if (width && (base == 0 || base == 16) && sc_peek(s) == '0') {
                sc_next(s); width--; got = 1;
                if (width && sc_is_x(sc_peek(s))) { sc_next(s); width--; base = 16; }
                else if (base == 0) base = 8;
            }
            if (base == 0) base = 10;
            while (width && (d = sc_digit(sc_peek(s))) < base) {
                v = v * (unsigned long)base + (unsigned long)d;
                sc_next(s); width--; got = 1;
            }
            if (!got) break;
            if (neg) v = 0UL - v;
            if (!suppress) {
                if (lc == 'p') size = 2;
                sc_store_int(va_arg(ap, void *), size, v, neg);
                assigned++;
            }
            converted++;
        } else if (lc == 'c') {
            char *p = suppress ? 0 : va_arg(ap, char *);
            int n = 0;
            while (n < width && sc_peek(s) != SC_EOF) {
                if (p) p[n] = (char)sc_peek(s);
                sc_next(s); n++;
            }
            if (n < width) { input_failure = 1; break; }
            if (p) assigned++;
            converted++;
        } else if (lc == 's' || conv == '[') {
            unsigned char set[32];
            int i, neg = 0, n = 0;
            if (conv == '[') {
                for (i = 0; i < 32; i++) set[i] = 0;
                if (*f == '^') { neg = 1; f++; }
                if (*f == ']') { set[']' >> 3] |= 1 << (']' & 7); f++; }
                while (*f && *f != ']') {
                    int lo = *f, hi = *f;
                    if (f[1] == '-' && f[2] && f[2] != ']') { hi = f[2]; f += 2; }
                    for (i = lo; i <= hi; i++) set[i >> 3] |= 1 << (i & 7);
                    f++;
                }
                if (*f == ']') f++;
            }
            char *p = suppress ? 0 : va_arg(ap, char *);
            while (n < width) {
                int ch = sc_peek(s);
                if (ch == SC_EOF) break;
                if (conv == '[') {
                    int in = (set[ch >> 3] >> (ch & 7)) & 1;
                    if (in == neg) break;
                } else if (sc_space(ch)) break;
                if (p) p[n] = (char)ch;
                sc_next(s); n++;
            }
            if (n == 0) break;
            if (p) { p[n] = 0; assigned++; }
            converted++;
        } else if (lc == 'f' || lc == 'e' || lc == 'g' || lc == 'a') {
            /* without the float scanf (scanf_float.c) a matching failure */
            float *p = suppress ? 0 : va_arg(ap, float *);
            if (!__scan_float(s, width, p)) break;
            if (p) assigned++;
            converted++;
        } else {
            break;      /* unknown conversion */
        }
    }

    if (src.la >= 0) __scan_ungetc(s, src.la);
    if (input_failure && converted == 0) return SC_EOF;
    return assigned;
}
