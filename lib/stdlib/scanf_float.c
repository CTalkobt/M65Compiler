/* scanf_float.c — %f %e %g for the scanf family (cc45 / MEGA65)
 *
 * The strong __scan_float, which replaces the scanf engine's weak stub
 * (scanf_core.c). cc45 references scanf_float (the sentinel below) when a
 * scanf, fscanf or sscanf call passes a float pointer, or a unit with float
 * code calls vscanf, vfscanf or vsscanf, which links this member.
 */

#pragma cc45 signed_default

#include "scanf_core.h"

float strtof(char *nptr, char **endptr);

/* Referenced by cc45 (see above) */
void scanf_float(void) {}

static int sc_is_e(int c) {
    return c == 0x45 || c == 0x65 || c == 0xC5;
}

/* [sign] digits [. digits] [e [sign] digits], at most width characters,
 * into *dest (not stored when dest is NULL); 1 when a number was read */
__fastcall__ int __scan_float(struct __scan_src *s, int width, float *dest) {
    char buf[40];
    int n = 0, digits = 0;
    int ch = sc_peek(s);
    if (width && (ch == '+' || ch == '-')) { buf[n++] = (char)ch; sc_next(s); width--; }
    while (width && n < 38 && (ch = sc_peek(s)) >= '0' && ch <= '9') {
        buf[n++] = (char)ch; sc_next(s); width--; digits++;
    }
    if (width && n < 38 && sc_peek(s) == '.') {
        buf[n++] = '.'; sc_next(s); width--;
        while (width && n < 38 && (ch = sc_peek(s)) >= '0' && ch <= '9') {
            buf[n++] = (char)ch; sc_next(s); width--; digits++;
        }
    }
    if (!digits) return 0;
    if (width && n < 36 && sc_is_e(sc_peek(s))) {
        buf[n++] = 'e'; sc_next(s); width--;
        ch = sc_peek(s);
        if (width && (ch == '+' || ch == '-')) { buf[n++] = (char)ch; sc_next(s); width--; }
        while (width && n < 38 && (ch = sc_peek(s)) >= '0' && ch <= '9') {
            buf[n++] = (char)ch; sc_next(s); width--;
        }
    }
    buf[n] = 0;
    if (dest) *dest = strtof(buf, 0);
    return 1;
}
