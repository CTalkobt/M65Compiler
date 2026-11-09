/* basic_key.c — basic45 --compile's KEY, FONT, CHAR DEF and DECBIN (#369),
 * as BASIC 65's (b65.src V920413: key, Key_List, list_special, Key_Change,
 * key_restore, fontsub, chardef, bindec). The keys are the editor's (its
 * table at $1000: 16 lengths, then the definitions; _kyndx $D1, _locks $F7
 * in the ROM's base page), changed through the KERNAL's _pfkey ($FF68).
 * Arguments: _bas_n, _bas_m, _bas_sa; results in _bas_n / __float_a.
 */

extern int _bas_n, _bas_m;
extern unsigned char *_bas_sa;
extern unsigned char _float_a[5];
extern void _bas_error(void);
extern void _bas_out(void);
extern void _float_ultof(void);

#define ROMBP(a) (*(volatile unsigned char __far *)(unsigned long)(a))
#define KYNDX 0xD1
#define LOCKS 0xF7
#define PKY ((volatile unsigned char *)0x1000)

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}
static void out(unsigned char c) { __jsr_raw(_bas_out, .A = c); }

static void waitkey(void) {
    while (ROMBP(KYNDX)) {}                 /* (a function key's text still going out) */
}

/* KEY ON (0), KEY OFF (1), KEY RESTORE (2): _bas_m */
void _bas_keymode(void) {
    waitkey();
    if (_bas_m == 1) ROMBP(LOCKS) |= 0x20;
    else {
        ROMBP(LOCKS) &= ~0x20;
        if (_bas_m == 2 && __jsr(0xFF68, .X = 0).Carry) raise(16);
    }
}

/* KEY n, string: key _bas_n (0-16; 0: the defaults again) := _bas_sa */
void _bas_keydef(void) {
    unsigned char n = (unsigned char)_bas_n;
    waitkey();
    if (n > 16) raise(14);
    /* (_pfkey reads the string's address, 3 bytes, at A in its base page) */
    ROMBP(6) = (unsigned char)(unsigned int)(_bas_sa + 1);
    ROMBP(7) = (unsigned char)((unsigned int)(_bas_sa + 1) >> 8);
    ROMBP(8) = 0;
    ROMBP(9) = 0;
    if (__jsr(0xFF68, .A = 6, .X = n, .Y = _bas_sa[0]).Carry) raise(16);
}

static void number(unsigned char n) {
    if (n >= 100) out('0' + n / 100);
    if (n >= 10) out('0' + n / 10 % 10);
    out('0' + n % 10);
}

/* KEY alone: each key defined as KEY n,"text"+CHR$(13)... */
void _bas_keylist(void) {
    /* (the letters as bytes: cc45's 'K' is PETSCII $CB) */
    static const unsigned char keydat[7] = {'(', '$', 0x52, 0x48, 0x43, '+', '"'};
    unsigned char k, y = 0;
    waitkey();
    for (k = 1; k <= 16; k++) {
        unsigned char size = PKY[k - 1], x;
        if (!size) continue;
        out(0x4B); out(0x45); out(0x59); out(' ');
        number(k);
        out(',');
        x = 7;
        do {
            unsigned char c = PKY[16 + y++];
            if (c == 13 || c == 0x8D || c == 0x22 || c == 27) {
                while (x >= 3) { out(keydat[x - 3]); x--; }
                number(c);
                out(')');
                x = 8;
            } else {
                if (x < 8) out('"');
                else if (x == 8) { out('+'); out('"'); }
                out(c);
                x = 9;
            }
        } while (--size);
        if (x >= 9) out('"');
        out(0x8D);
    }
}

/* FONT A-F (_bas_n 0-5): the case (A-C upper, D-F lower) and the font
 * (A / D, B / E, C / F) copied to the character generator */
void _bas_font(void) {
    static const unsigned char page[3] = {0x90, 0xD0, 0xD0}, bank[3] = {0x02, 0x03, 0x02};
    unsigned char y = (unsigned char)_bas_n;
    unsigned char __far *s;
    unsigned char __far *d = (unsigned char __far *)0xFF7E000UL;
    unsigned int k;
    if (y >= 3) { y -= 3; out(0x0E); } else out(0x8E);
    s = (unsigned char __far *)((unsigned long)bank[y] << 16 | (unsigned int)page[y] << 8);
    for (k = 0; k < 4096; k++) d[k] = s[k];
}

/* CHAR DEF index, byte, ...: _bas_chrdefi the character (0-511, its low 16
 * bits: ?ILLEGAL QUANTITY from 512), _bas_chrdefb each byte on */
static unsigned char __far *cdef;
void _bas_chrdefi(void) {
    unsigned int i = (unsigned int)_bas_n;
    if ((i >> 8) >= 2) raise(14);
    cdef = (unsigned char __far *)(0xFF7E000UL + (unsigned long)i * 8);
}
void _bas_chrdefb(void) {
    *cdef++ = (unsigned char)_bas_n;
}

/* DECBIN(s$): its leading binary digits (up to 32), unsigned */
void _bas_decbin(void) {
    unsigned char *s = _bas_sa, n = s[0], k;
    unsigned long v = 0;
    for (k = 1; k <= n; k++) {
        unsigned char d = (unsigned char)(s[k] - '0');
        if (d >= 2) break;
        v = v << 1 | d;
    }
    __jsr_raw(_float_ultof, .A = (unsigned char)v, .X = (unsigned char)(v >> 8), .Y = (unsigned char)(v >> 16),
              .Z = (unsigned char)(v >> 24));
}
