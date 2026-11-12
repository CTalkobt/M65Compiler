/* test_basic_fns_xemu.c — basic45 --compile's HEX$ / STRBIN$, DEC, INSTR and
 * AND / OR / NOT / XOR's integer in assembly (basic_fns.s45, #369) against
 * their former C (basic_fn.c, copied here as the reference): the same
 * strings, values, positions and errors for each input. One change from the
 * C: below -2^31 the interpreter's integer is the magnitude (HEX$(-3E9) is
 * B2D05E00), which the C's float arithmetic got wrong; the reference here
 * has the interpreter's rule.
 *
 * Results at $C0C0: [0] HEX$ / STRBIN$ differences, [1] DEC's, [2] INSTR's,
 * [3] the integer's (0 = pass each), [4] $AA
 */
#include <math.h>
#include <setjmp.h>
#include <string.h>

extern unsigned char *_bas_sa, *_bas_sb, *_bas_sr;
extern int _bas_n, _bas_m;
extern unsigned char _float_a[5];
extern void _bas_hex(void), _bas_dec(void), _bas_instr(void), _bas_ayintw(void);

#define OUT ((volatile unsigned char *)0xC0C0)

/* the run time's errors: _bas_error replaced, back to the test */
static jmp_buf back;
static int raised;
void _bas_error(void) { raised = _bas_n; longjmp(back, 1); }

static float geta(void) { float f; memcpy(&f, _float_a, 5); return f; }
static void seta(float f) { memcpy(_float_a, &f, 5); }

/* --- the reference: basic_fn.c as it was --- */
static void ref_raise(int n) { _bas_n = n; _bas_error(); }
static unsigned long int32(void) {
    float f = floorf(geta());
    if (f >= 4294967296.0 || f <= -4294967296.0) ref_raise(14);
    if (f >= 2147483648.0) return (unsigned long)(long)(f - 2147483648.0) + 0x80000000UL;
    if (f < -2147483648.0) {
        /* (the interpreter's: the magnitude; exact, both of exponent $A0) */
        float m = 0.0 - f;
        return (unsigned long)(long)(m - 2147483648.0) + 0x80000000UL;
    }
    return (unsigned long)(long)f;
}
static void ref_hex(void) {
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
static void ref_dec(void) {
    unsigned char *s = _bas_sa, n = s[0], i;
    unsigned long v = 0;
    for (i = 1; i <= n; i++) {
        unsigned char c = s[i], d;
        if (c >= 0x30 && c <= 0x39) d = c - 0x30;
        else if (c >= 0x41 && c <= 0x46) d = c - 0x41 + 10;
        else break;
        v = v << 4 | d;
    }
    seta((float)(v >> 1) * 2.0 + (float)(v & 1));
}
static void ref_instr(void) {
    unsigned char *h = _bas_sa + 1, *nd = _bas_sb + 1;
    unsigned char hl = _bas_sa[0], nl = _bas_sb[0], pos = (unsigned char)_bas_n - 1, z, y;
    _bas_n = 0;
    if (!hl || !nl) return;
    if (nd[0] == 92) {
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
                        z--;
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
static void ref_ayintw(void) {
    float f = floorf(geta());
    if (f < -32768.0 || f >= 65536.0) ref_raise(14);
    if (f < 0.0) _bas_n = (int)(long)f;
    else _bas_n = (int)(unsigned int)(unsigned long)(long)f;
}

/* one call's outcome: the error raised (0 none), __bas_n, the result string, __float_a */
struct out { int err, n; unsigned char s[40], fa[5]; };
static unsigned char rbuf[40];
static void run(void (*f)(void), struct out *o) {
    memset(rbuf, 0xEE, sizeof rbuf);
    raised = 0;
    if (!setjmp(back)) f();
    o->err = raised;
    o->n = _bas_n;
    memcpy(o->s, rbuf, sizeof rbuf);
    memcpy(o->fa, _float_a, 5);
}

/* the asm and the reference on the same inputs: 1 when they differ
 * (keepfa: __float_a is the result; else it is the reference's scratch) */
static unsigned char differ(void (*a)(void), void (*r)(void), int n, int m, const unsigned char *fa, unsigned char keepfa) {
    struct out x, y;
    _bas_sr = rbuf;
    _bas_n = n; _bas_m = m; if (fa) memcpy(_float_a, fa, 5);
    run(a, &x);
    _bas_n = n; _bas_m = m; if (fa) memcpy(_float_a, fa, 5);
    run(r, &y);
    if (!keepfa) { memset(x.fa, 0, 5); memset(y.fa, 0, 5); }
    return memcmp(&x, &y, sizeof x) != 0;
}

static unsigned char sa[40], sb[40];
static void pstr(unsigned char *d, const char *s) { d[0] = (unsigned char)strlen(s); memcpy(d + 1, s, d[0]); }

static const float nums[] = {
    0.0, 1.0, -1.0, 0.5, -0.5, 255.0, 256.0, 65535.0, 65535.9, 65536.0, -32768.0, -32768.5, -32769.0, 123456.0,
    2147483647.0, 2147483648.0, 3000000000.0, 4294967295.0, 4294967296.0, -2147483648.0, -2147483649.0,
    -4294967295.0, -4294967296.0, -3000000000.0, -4294967294.0, 1e-30, -1e-30, 1e30, -1e30, 12.75, -12.75, 999.999, -0.0001,
};
static const char *const hexs[] = { "", "0", "FF", "ff", "1A2B", "12345678", "123456789", "FFFFFFFF", "7G", "G7", "00000001", "A", "F0F0F0F0F" };
static const char *const hay[] = { "HELLO WORLD", "AAAAB", "", "ABCABC", "MISSISSIPPI", "X" };
static const char *const ndl[] = { "O", "WORLD", "AB", "", "ISS", "Z", "\\A*B", "\\.L", "\\L*O", "\\S*I", "\\", "\\X*", "\\..", "HELLO WORLD!" };

int main(void) {
    unsigned char i, j, k, bad[4] = {0, 0, 0, 0};
    float f;
    for (i = 0; i < sizeof nums / sizeof nums[0]; i++) {
        f = nums[i];
        bad[0] += differ(_bas_hex, ref_hex, 0, 0, (unsigned char *)&f, 0);
        bad[0] += differ(_bas_hex, ref_hex, 0, 1, (unsigned char *)&f, 0);
        bad[3] += differ(_bas_ayintw, ref_ayintw, 0, 0, (unsigned char *)&f, 0);
    }
    for (i = 0; i < sizeof hexs / sizeof hexs[0]; i++) {
        pstr(sa, hexs[i]);
        _bas_sa = sa;
        bad[1] += differ(_bas_dec, ref_dec, 0, 0, 0, 1);
    }
    for (i = 0; i < sizeof hay / sizeof hay[0]; i++)
        for (j = 0; j < sizeof ndl / sizeof ndl[0]; j++)
            for (k = 0; k < 5; k++) {
                pstr(sa, hay[i]);
                pstr(sb, ndl[j]);
                _bas_sa = sa;
                _bas_sb = sb;
                bad[2] += differ(_bas_instr, ref_instr, k, 0, 0, 1);
            }
    for (i = 0; i < 4; i++) OUT[i] = bad[i];
    OUT[4] = 0xAA;
    return 0;
}
