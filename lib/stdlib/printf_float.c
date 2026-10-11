/* printf_float.c — Float-aware printf/sprintf for cc45 / MEGA65
 *
 * Provides strong _printf and _sprintf that override the weak integer-only
 * versions. Also exports _printf_float (the sentinel symbol) which triggers
 * the linker to pull this module when float args are passed to variadic calls.
 *
 * Supports all integer format specifiers plus %f for float/double.
 * Float values are 5 bytes (CBM 40-bit) pushed on the stack.
 */

#include <stdarg.h>

int itoa(int value, char *str, int base);

/* _printf_float: convert float to decimal string in buf.
 * Returns number of chars written. Default precision = 6.
 * This is also the sentinel symbol that triggers float printf linkage.
 */
int printf_float(char *buf, float val) {
    char *out = buf;
    int neg = 0;

    /* Handle negative */
    if (val < 0.0) {
        neg = 1;
        val = 0.0 - val;
    }

    /* Handle zero */
    if (val == 0.0) {
        if (neg) { *out = '-'; out = out + 1; }
        *out = '0'; out = out + 1;
        *out = '.'; out = out + 1;
        *out = '0'; out = out + 1;
        *out = '0'; out = out + 1;
        *out = '0'; out = out + 1;
        *out = '0'; out = out + 1;
        *out = '0'; out = out + 1;
        *out = '0'; out = out + 1;
        *out = 0;
        return (int)(out - buf);
    }

    if (neg) { *out = '-'; out = out + 1; }

    /* Extract integer part */
    int ipart = (int)val;
    float frac = val - (float)ipart;

    /* Integer part to string */
    char itmp[12];
    itoa(ipart, itmp, 10);
    int i = 0;
    while (itmp[i]) {
        /* skip minus from itoa (we handle sign ourselves) */
        if (itmp[i] != '-') { *out = itmp[i]; out = out + 1; }
        i = i + 1;
    }

    /* Decimal point */
    *out = '.'; out = out + 1;

    /* Fractional part: 6 digits */
    int d = 0;
    while (d < 6) {
        frac = frac * 10.0;
        int digit = (int)frac;
        if (digit > 9) digit = 9;
        if (digit < 0) digit = 0;
        *out = (char)(digit + 48);
        out = out + 1;
        frac = frac - (float)digit;
        d = d + 1;
    }

    *out = 0;
    return (int)(out - buf);
}

/* --- printf with %f support (strong, overrides weak integer-only version) --- */

int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt, va_list ap,
              int (*ffmt)(char *, float));
void __vformat_con_put(int c, void *ctx);

int printf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int count = __vformat(__vformat_con_put, 0, fmt, ap, printf_float);
    va_end(ap);
    return count;
}
