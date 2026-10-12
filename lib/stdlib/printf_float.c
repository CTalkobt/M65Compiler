/* printf_float.c — Float-aware printf/sprintf for cc45 / MEGA65
 *
 * Provides strong _printf and _sprintf that override the weak integer-only
 * versions. Also exports _printf_float (the sentinel symbol) which triggers
 * the linker to pull this module when float args are passed to variadic calls.
 *
 * The digits come from the BASIC 65 ROM's own number formatting (FOUT, what
 * PRINT and STR$ use, via __float_fout in float_rom.s45), so every value of
 * the CBM 40-bit format (about +-1.7E38, 9-10 significant digits) prints.
 * They are turned into C's %f, %e and %g (and %F %E %G) with the precision
 * (default 6), rounding half up, and the # flag.
 */

#include <stdarg.h>

extern float _float_a;
extern char _float_str[];
void _float_fout(void);

#define PF_DIGITS 12   /* significant digits kept from the ROM's text */
#define PF_MAXPREC 20  /* larger precisions are clamped (buffer size) */

/* The value as 0.d1 d2 d3 ... x 10^exp; nd == 0 for zero */
static signed char pf_d[PF_DIGITS];
static signed char pf_nd;
static signed int pf_exp;

/* Parse the ROM's text: [ -]digits[.digits][E[+-]digits]. nonzero: the
 * value is not 0 (FOUT rounds 9.99999999...E19 up to the text " 0E19": a
 * carry it loses; that is 10E19) */
static signed char pf_parse(signed char nonzero) {
    char *s = _float_str;
    signed char neg = 0, seenDot = 0;
    signed int intDigits = 0, lead = 0;
    pf_nd = 0;
    pf_exp = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;
    for (; *s; s++) {
        char c = *s;
        if (c == '.') { seenDot = 1; continue; }
        if (c < '0' || c > '9') break;
        if (pf_nd == 0 && c == '0') {
            /* leading zeros: before the point they don't count, after it
               they move the exponent */
            if (seenDot) lead--;
            continue;
        }
        if (pf_nd < PF_DIGITS) pf_d[pf_nd++] = (signed char)(c - '0');
        if (!seenDot) intDigits++;
    }
    if (*s == 'E' || *s == 'e') {
        signed char eneg = 0;
        signed int e = 0;
        s++;
        if (*s == '-') { eneg = 1; s++; }
        else if (*s == '+') s++;
        while (*s >= '0' && *s <= '9') { e = e * 10 + (*s - '0'); s++; }
        pf_exp = eneg ? -e : e;
    }
    /* drop trailing zero digits */
    while (pf_nd > 0 && pf_d[pf_nd - 1] == 0) pf_nd--;
    if (pf_nd == 0 && nonzero) {
        /* the lost carry: "0E19" is 10E19 = 0.1 x 10^21 */
        pf_d[0] = 1;
        pf_nd = 1;
        pf_exp = pf_exp + 2;
        return neg;
    }
    pf_exp = pf_exp + (pf_nd ? (intDigits ? intDigits : lead) : 0);
    return neg;
}

/* Keep `sig` significant digits, rounding half up (sig may be <= 0) */
static void pf_round(signed int sig) {
    signed int k;
    if (pf_nd == 0 || sig >= pf_nd) return;
    if (sig < 0) { pf_nd = 0; return; }
    if (pf_d[sig] < 5) {
        pf_nd = (signed char)sig;
    } else {
        k = sig - 1;
        while (k >= 0 && pf_d[k] == 9) k--;
        if (k < 0) {
            /* 9.99 -> 10.0: one digit more in front */
            pf_d[0] = 1;
            pf_nd = 1;
            pf_exp++;
            return;
        }
        pf_d[k]++;
        pf_nd = (signed char)(k + 1);
    }
    while (pf_nd > 0 && pf_d[pf_nd - 1] == 0) pf_nd--;
}

/* The digit at position i of 0.d1 d2 ... (0 beyond the end) */
static char pf_digit(signed int i) {
    return (char)('0' + ((i >= 0 && i < pf_nd) ? pf_d[i] : 0));
}

/* buf: the formatted value (sign included). conv: f e g F E G.
 * prec: digits after the point (%f %e) or significant digits (%g); < 0 = 6.
 * alt: the # flag (keep the point, keep %g's trailing zeros).
 * Returns the length. buf needs 1 + 39 + 1 + PF_MAXPREC + 1 bytes. */
int printf_float(char *buf, float val, int conv, int prec, int alt) {
    char *out = buf;
    signed int i, e, P;
    /* (letters compared, not case arithmetic: strings are PETSCII) */
    char upper = (conv == 'E' || conv == 'G' || conv == 'F');
    char c = (conv == 'E' || conv == 'e') ? 'e' : (conv == 'G' || conv == 'g') ? 'g' : 'f';
    signed char strip = 0;

    _float_a = val;
    _float_fout();
    if (pf_parse(val != 0.0)) *out++ = '-';

    if ((signed int)prec < 0) prec = 6;
    if ((signed int)prec > PF_MAXPREC) prec = PF_MAXPREC;

    if (c == 'g') {
        P = prec ? (signed int)prec : 1;
        pf_round(P);
        e = pf_nd ? pf_exp - 1 : 0;
        if (e < -4 || e >= P) { c = 'e'; prec = P - 1; }
        else { c = 'f'; prec = P - 1 - e; }
        strip = !alt;
    }

    if (c == 'e') {
        pf_round((signed int)prec + 1);
        e = pf_nd ? pf_exp - 1 : 0;
        *out++ = pf_digit(0);
        if (prec || alt) *out++ = '.';
        for (i = 1; i <= (signed int)prec; i++) *out++ = pf_digit(i);
        if (strip && prec) {
            while (out[-1] == '0') out--;
            if (out[-1] == '.') out--;
        }
        *out++ = upper ? 'E' : 'e';
        if (e < 0) { *out++ = '-'; e = -e; } else *out++ = '+';
        if (e >= 10) *out++ = (char)('0' + e / 10);
        else *out++ = '0';
        *out++ = (char)('0' + e % 10);
    } else {
        /* f: the digits before the point are positions 0 .. exp-1 */
        pf_round(pf_exp + (signed int)prec);
        if (pf_nd == 0 || pf_exp <= 0) {
            *out++ = '0';
        } else {
            for (i = 0; i < pf_exp; i++) *out++ = pf_digit(i);
        }
        if (prec || alt) *out++ = '.';
        for (i = 0; i < (signed int)prec; i++) *out++ = pf_digit(pf_exp + i);
        if (strip && (prec || alt)) {
            while (out[-1] == '0') out--;
            if (out[-1] == '.') out--;
        }
    }
    *out = 0;
    return (int)(out - buf);
}

/* --- printf with %f support (strong, overrides weak integer-only version) --- */

int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt, va_list ap,
              int (*ffmt)(char *, float, int, int, int));
void __vformat_con_put(int c, void *ctx);

int printf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int count = __vformat(__vformat_con_put, 0, fmt, ap, printf_float);
    va_end(ap);
    return count;
}
