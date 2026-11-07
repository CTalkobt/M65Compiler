/* basic_fn.c — functions of BASIC 65 programs compiled by basic45 --compile (#369)
 *
 * (Characters as PETSCII codes: cc45 makes a C 'E' a shifted letter.)
 * As BASIC 65's (b65.src): the arguments in __bas_sa / __bas_sb (strings),
 * __bas_n / __bas_m (16-bit numbers), __float_a (a number); a string result
 * into __bas_sr's buffer, a number into __float_a or __bas_n.
 *   _bas_val     VAL(__bas_sa): its get_number's syntax (a sign, digits, one
 *                point, one E with a sign right after it; it ends at another
 *                character; a second point or E, or an E without digits: 0;
 *                a sign alone: -0); the value by strtof (correctly rounded)
 *   _bas_hex     HEX$(__float_a): 4 digits for 0..65535, else 8 (a negative
 *                number as 32 bits); STRBIN$: 8, 16 or 32 binary digits
 *   _bas_dec     DEC(__bas_sa): hex digits to the first other character
 *   _bas_instr   INSTR(__bas_sa, __bas_sb, __bas_n): the position, 0 for none;
 *                a needle starting with a pound sign is a pattern ('.' any
 *                character, 'c*' none or more of c)
 *   _bas_rpt     RPT$(__bas_sa, __bas_n)
 *   _bas_sleep   SLEEP __float_a seconds (TI)
 *   _bas_ayintw  __float_a rounded down to an integer -32768..65535 (AND, OR,
 *                NOT, XOR): __bas_n its 16 bits; ?ILLEGAL QUANTITY outside
 */
#include <stdlib.h>
#include <math.h>

extern unsigned char *_bas_sa, *_bas_sb, *_bas_sr;
extern int _bas_n, _bas_m;
extern unsigned char _float_a[5];
extern void _bas_error(void);
extern void _bas_ti(void);

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

static float geta(void) {
    float f;
    unsigned char k;
    for (k = 0; k < 5; k++) ((unsigned char *)&f)[k] = _float_a[k];
    return f;
}

static void seta(float f) {
    unsigned char k;
    for (k = 0; k < 5; k++) _float_a[k] = ((unsigned char *)&f)[k];
}

void _bas_val(void) {
    unsigned char *s = _bas_sa, n = s[0], i = 1, start, neg = 0, dot = 0, e = 0, edigits = 0;
    char buf[40];
    unsigned char k = 0;
    float f;
    while (i <= n && s[i] == 0x20) i++;
    if (i <= n && (s[i] == 0x2B || s[i] == 0x2D)) { neg = s[i] == 0x2D; i++; }
    start = i;
    for (; i <= n; i++) {
        unsigned char c = s[i];
        if (c == 0x2E) {
            if (dot || e) { seta(0.0); return; }
            dot = 1;
        } else if (c == 0x45) {          /* E (PETSCII: a C 'E' is shifted) */
            if (e) { seta(0.0); return; }
            e = 1;
        } else if (c == 0x2B || c == 0x2D) {
            if (!e) break;
            if (s[i - 1] != 0x45) { seta(0.0); return; }
        } else if (c >= 0x30 && c <= 0x39) {
            if (e) edigits = 1;
        } else break;
        if (k < sizeof buf - 1) buf[k++] = c;
    }
    buf[k] = 0;
    if (e && !edigits) { seta(0.0); return; }
    /* (one character, "." or "0": zero without the sign) */
    if (i - start == 1 && (buf[0] == 0x2E || buf[0] == 0x30)) { seta(0.0); return; }
    f = strtof(buf, 0);
    if (neg) {
        f = -f;
        if (f == 0.0) { seta(0.0); _float_a[1] = 0x80; return; }   /* -0 */
    }
    seta(f);
}

/* __float_a rounded down, as 32 bits (|x| < 2^32, ?ILLEGAL QUANTITY else) */
static unsigned long int32(void) {
    float f = floorf(geta());
    if (f >= 4294967296.0 || f <= -4294967296.0) raise(14);
    if (f >= 2147483648.0) return (unsigned long)(long)(f - 2147483648.0) + 0x80000000UL;
    if (f < -2147483648.0) return (unsigned long)(long)(f + 4294967296.0);
    return (unsigned long)(long)f;
}

/* __bas_m: 0 HEX$, 1 STRBIN$ */
void _bas_hex(void) {
    unsigned long u = int32();
    unsigned char *r = _bas_sr, bits, k, d;
    if (_bas_m) bits = u <= 0xFF ? 8 : u <= 0xFFFF ? 16 : 32;
    else bits = u <= 0xFFFF ? 16 : 32;
    if (_bas_m) {
        r[0] = bits;
        for (k = 0; k < bits; k++) r[1 + k] = (u >> (bits - 1 - k)) & 1 ? 0x31 : 0x30;
    } else {
        r[0] = bits / 4;
        for (k = 0; k < bits / 4; k++) {
            d = (u >> (bits - 4 - 4 * k)) & 15;
            r[1 + k] = d < 10 ? 0x30 + d : 0x41 + d - 10;
        }
    }
}

void _bas_dec(void) {
    unsigned char *s = _bas_sa, n = s[0], i;
    unsigned long v = 0;
    for (i = 1; i <= n; i++) {
        unsigned char c = s[i], d;
        if (c >= 0x30 && c <= 0x39) d = c - 0x30;
        else if (c >= 0x41 && c <= 0x46) d = c - 0x41 + 10;      /* A-F */
        else break;
        v = v << 4 | d;
    }
    seta((float)(v >> 1) * 2.0 + (float)(v & 1));    /* (unsigned: above 2^31 too) */
}

void _bas_instr(void) {
    unsigned char *h = _bas_sa + 1, *nd = _bas_sb + 1;
    unsigned char hl = _bas_sa[0], nl = _bas_sb[0], pos = (unsigned char)_bas_n - 1, z, y;
    _bas_n = 0;
    if (!hl || !nl) return;
    if (nd[0] == 92) {
        /* a pattern: '.' any character, 'c*' none or more of c */
        nd++;
        if (!--nl) return;
        if (pos >= hl || hl < nl) return;
        for (; pos < hl; pos++) {
            unsigned char cnt = hl - pos;
            z = 0;
            y = 0;
            for (;;) {
                unsigned char c = nd[y];
                if (c != '.') {
                    if (y + 1 < nl && nd[y + 1] == '*') {
                        while (z < hl - pos && h[pos + z] == c) z++;
                        z--;            /* (the last matched; none: 255, the inz below makes it 0) */
                        y++;
                    } else if (h[pos + z] != c) break;
                }
                if (++y == nl) { _bas_n = pos + 1; return; }
                if (++z >= cnt) return;
            }
        }
        return;
    }
    if (pos >= hl || nl > hl - pos) return;
    for (; pos + nl <= hl; pos++) {
        for (z = 0; z < nl && h[pos + z] == nd[z]; z++) {}
        if (z == nl) { _bas_n = pos + 1; return; }
    }
}

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

void _bas_sleep(void) {
    float wait = geta(), until;
    if (wait < 0.0) raise(14);
    _bas_ti();
    until = geta() + wait;
    do _bas_ti(); while (geta() < until);
}

void _bas_ayintw(void) {
    float f = floorf(geta());
    if (f < -32768.0 || f >= 65536.0) raise(14);
    if (f < 0.0) _bas_n = (int)(long)f;
    else _bas_n = (int)(unsigned int)(unsigned long)(long)f;
}
