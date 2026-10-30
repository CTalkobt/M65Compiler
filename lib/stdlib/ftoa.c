/* ftoa.c — float to text (<stdlib.h>, as oscar64's): f written at s as
 * printf's %g writes it (6 significant digits, an exponent outside
 * 1e-4 .. 1e6), by the printf family's float formatter */

/* stdlib_asm/pf_format.s45, the ZP convention */
__fastcall__ int __pf_format(char *buf, float val, int conv, int prec, int alt);

void ftoa(float f, char *s) {
    __pf_format(s, f, 'g', 6, 0);
}
