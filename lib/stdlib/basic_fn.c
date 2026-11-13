/* basic_fn.c — functions of BASIC 65 programs compiled by basic45 --compile (#369)
 *
 * (Characters as PETSCII codes: cc45 makes a C 'E' a shifted letter.)
 * As BASIC 65's (b65.src): the arguments in __bas_sa / __bas_sb (strings),
 * __bas_n / __bas_m (16-bit numbers), __float_a (a number); a string result
 * into __bas_sr's buffer, a number into __float_a or __bas_n.
 *   (VAL: basic_fin.s45, in assembly)
 *   (HEX$, STRBIN$, DEC, INSTR and AND / OR / NOT / XOR's integer: basic_fns.s45)
 *   (SLEEP: basic_ti.s45)
 *   _bas_rpt     RPT$(__bas_sa, __bas_n)
 */
#include <stdlib.h>

extern unsigned char *_bas_sa, *_bas_sb, *_bas_sr;
extern int _bas_n, _bas_m;
extern void _bas_error(void);

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

/* (_bas_val: basic_fin.s45) */

void _bas_rpt(void) {
    unsigned char *s = _bas_sa, *r = _bas_sr, n = (unsigned char)_bas_n, k, i, l = s[0];
    unsigned int total = (unsigned int)l * n;
    if (total > 255) raise(23);
    r[0] = (unsigned char)total;
    for (k = 0, i = 1; k < n; k++) {
        unsigned char j;
        for (j = 0; j < l; j++) r[i++] = s[1 + j];
    }
}
