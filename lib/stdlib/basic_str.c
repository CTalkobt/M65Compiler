/* basic_str.c — the strings of BASIC 65 programs compiled by basic45 --compile (#369)
 *
 * A string is a pointer to a length byte (0..255) and its characters. A
 * variable points to a block of its own on the heap (malloc), or to
 * __bas_empty. A string expression's result goes to a buffer the compiled
 * code chooses (__bas_sr: 256 bytes). The arguments are in variables
 * (basic_rt.s45): __bas_sa, __bas_sb (strings), __bas_n, __bas_m (numbers).
 * The C names here are those symbols without the first underscore.
 */
#include <stdlib.h>
#include <string.h>

extern unsigned char *_bas_sa, *_bas_sb, *_bas_sr;
extern int _bas_n, _bas_m;
extern unsigned char _float_str[];
extern void _bas_error(void);
extern void _float_fout(void);

/* the empty string */
unsigned char _bas_empty[1];

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

/* *__bas_sr (a variable) := a copy of __bas_sa; its old block freed */
void _bas_sset(void) {
    unsigned char **var = (unsigned char **)_bas_sr;
    unsigned char *s = _bas_sa;
    unsigned char *old = *var;
    unsigned char *p = _bas_empty;
    if (s[0]) {
        p = (unsigned char *)malloc(s[0] + 1);
        if (!p) raise(16);
        memcpy(p, s, s[0] + 1);
    }
    if (old != _bas_empty && old) free(old);
    *var = p;
}

/* __bas_sr := __bas_sa + __bas_sb (?STRING TOO LONG over 255) */
void _bas_scat(void) {
    unsigned char *a = _bas_sa, *b = _bas_sb, *r = _bas_sr;
    unsigned int n = a[0] + b[0];
    if (n > 255) raise(23);
    /* (r may be a: b's characters go after a's) */
    memmove(r + 1 + a[0], b + 1, b[0]);
    if (r != a) memmove(r + 1, a + 1, a[0]);
    r[0] = (unsigned char)n;
}

/* __bas_n := -1, 0, 1 as __bas_sa is before, equal to, after __bas_sb */
void _bas_scmp(void) {
    unsigned char *a = _bas_sa, *b = _bas_sb;
    unsigned char la = a[0], lb = b[0];
    unsigned char i;
    for (i = 0; i < la && i < lb; i++) {
        if (a[i + 1] != b[i + 1]) {
            _bas_n = a[i + 1] < b[i + 1] ? -1 : 1;
            return;
        }
    }
    _bas_n = la == lb ? 0 : (la < lb ? -1 : 1);
}

/* the n characters of a from position p (1-based) into r */
static void part(unsigned char *a, unsigned int p, unsigned int n) {
    unsigned char *r = _bas_sr;
    unsigned int len = a[0];
    if (p > len) n = 0;
    else if (n > len - p) n = len - p;
    memmove(r + 1, a + 1 + p, n);
    r[0] = (unsigned char)n;
}

/* LEFT$(__bas_sa, __bas_n) */
void _bas_left(void) {
    part(_bas_sa, 0, _bas_n);
}

/* RIGHT$(__bas_sa, __bas_n) */
void _bas_right(void) {
    unsigned int len = _bas_sa[0];
    unsigned int n = _bas_n;
    part(_bas_sa, n >= len ? 0 : len - n, n);
}

/* MID$(__bas_sa, __bas_n [, __bas_m]): position 1..255 (0: ?ILLEGAL QUANTITY) */
void _bas_mid(void) {
    if (_bas_n == 0) raise(14);
    part(_bas_sa, _bas_n - 1, _bas_m);
}

/* CHR$(__bas_n) */
void _bas_chr(void) {
    _bas_sr[0] = 1;
    _bas_sr[1] = (unsigned char)_bas_n;
}

/* STR$(__float_a): the number as PRINT shows it (without the cursor right) */
void _bas_str(void) {
    unsigned char n = 0;
    _float_fout();
    while (_float_str[n]) {
        _bas_sr[n + 1] = _float_str[n];
        n++;
    }
    _bas_sr[0] = n;
}

/* BASIC's stack: a GOSUB's return address (256 and up), a FOR loop's number
 * (1..255); __bas_forvar[n]: loop n's variable (basic45 makes it) */
unsigned int _bas_stk[128];
unsigned char _bas_sp;
extern unsigned char _bas_forvar[];

/* FOR loop __bas_n: a loop of the same variable on the stack (above the
 * last GOSUB) goes, with the entries above it (as BASIC's FOR) */
void _bas_forpush(void) {
    unsigned char id = (unsigned char)_bas_n;
    unsigned char v = _bas_forvar[id];
    unsigned char i = _bas_sp;
    while (i) {
        unsigned int e = _bas_stk[i - 1];
        if (e >= 256) break;
        if (_bas_forvar[e] == v) { _bas_sp = i - 1; break; }
        i--;
    }
    if (_bas_sp >= 128) raise(16);
    _bas_stk[_bas_sp++] = id;
}
