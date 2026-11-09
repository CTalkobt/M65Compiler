/* basic_using.c — basic45 --compile's PRINT USING (#369), as BASIC 65's
 * (b65.src V920413: pu_using, pu_parse, cprintf, cdecode, cwidth, pu_arg,
 * pu_round, pu_insert, pu_hex, pu_left / pu_right / pu_centre,
 * pu_blank_fill, pu_print), its 8-bit index arithmetic kept.
 *
 * One value a USING: _bas_sa the format (a length byte and its characters;
 * "" is ?SYNTAX ERROR), _bas_m 1: the string _bas_sb, else the number in
 * __float_a. A format: # a digit or a character, . the point (a number), >
 * / = first: a string right-justified / centred, %[0]wD %[0]wF %[0]w.pF
 * %[0]wX: a field of width w (p decimals; X: hexadecimal; 0: zero padding).
 * A number's digits are FOUT's (__float_fout); a number too wide is *s.
 */

extern int _bas_n, _bas_m;
extern unsigned char *_bas_sa, *_bas_sb;
extern unsigned char _float_a[5], _float_str[24];
extern void _bas_error(void);
extern void _bas_out(void);
extern void _float_fout(void);

#define PU_FLAG (*(volatile unsigned char __far *)0x8BUL)   /* the ROM's (FOUT's tiny bit) */

/* (the letters as the program's text holds them: cc45's 'D' is PETSCII $C4) */
#define L_D 0x44
#define L_F 0x46
#define L_X 0x58
#define L_A 0x41

static unsigned char buf[256], hulp, point, before, after, flag, type, width, prec, nlen, fix, frac, dot;
static const unsigned char *fmt;
static unsigned char fmtlen;

static void putc_(unsigned char c) { __jsr_raw(_bas_out, .A = c); }

/* cdecode: z at the character before; a number of one or two digits: x, the
 * character after it (z there); 0 when none (z one on) */
static unsigned char cdecode(unsigned char *z, unsigned char *x, unsigned char *a) {
    unsigned char c, d1;
    (*z)++;
    c = fmt[*z];
    if (c < '0' || c > '9') { *a = c; return 0; }
    d1 = c & 15;
    (*z)++;
    c = fmt[*z];
    if (c >= '0' && c <= '9') {
        *x = (unsigned char)((c & 15) + 10 * d1);
        (*z)++;
        *a = fmt[*z];
    } else {
        *x = d1;
        *a = c;
    }
    return 1;
}

/* cwidth: the field's # and . into the buffer at y */
static void cwidth(unsigned char *y) {
    unsigned char z = (unsigned char)(width - prec);
    do {
        buf[*y] = '#';
        before++;
        (*y)++;
    } while (--z);
    point = *y;
    if (prec) {
        buf[*y] = '.';
        (*y)++;
        z = prec;
        do {
            buf[*y] = '#';
            after++;
            (*y)++;
        } while (--z);
    }
    (*y)--;
}

/* cprintf: a % field at z; 1 parsed (z at its type letter, y its last byte) */
static unsigned char cprintf(unsigned char *z, unsigned char *y) {
    unsigned char z0 = *z, x = 0, a;
    width = prec = type = 0;
    (*z)++;
    if (fmt[*z] == '0') flag |= 8;
    else (*z)--;
    if (!cdecode(z, &x, &a)) { *z = z0; return 0; }
    width = x;
    if (a != L_D && a != L_F && a != L_X) {
        if (a != '.') { *z = z0; return 0; }
        if (cdecode(z, &x, &a)) prec = x;
        if (a != L_F) { *z = z0; return 0; }
    }
    type = a;
    cwidth(y);
    return 1;
}

static void parse(void) {
    unsigned char z = 0, y = 0, c;
    point = fmtlen;
    before = after = flag = type = 0;
    do {
        c = fmt[z];
        if (c == '%' && cprintf(&z, &y)) goto next;
        buf[y] = c;
        if (c == '.') { flag |= 0x80; point = z; }
        else if (c == '#') { if (flag & 0x80) after++; else before++; }
    next:
        y++;
        z++;
    } while (z < fmtlen);
    hulp = y;
}

static void blankfill(void) {
    unsigned char y = 0xFF;
    for (;;) {
        y++;
        if (y >= hulp) return;
        if (buf[y] == '#') buf[y] = ' ';
    }
}

/* the string from z + 1 into the #s from the left */
static void leftz(const unsigned char *s, unsigned char z) {
    unsigned char y = 0xFF;
    for (;;) {
        z++;
        if (z >= nlen) { blankfill(); return; }
        do {
            y++;
            if (y >= hulp) return;
        } while (buf[y] != '#');
        buf[y] = s[z];
    }
}

static void right(const unsigned char *s) {
    unsigned char z = nlen, y = hulp;
    buf[0] = 0;
    for (;;) {
        z--;
        if (z == 0xFF) { blankfill(); return; }
        do {
            y--;
            if (!y) return;
        } while (buf[y] != '#');
        buf[y] = s[z];
    }
}

static void centre(const unsigned char *s) {
    unsigned char a = (unsigned char)(before + after), bl, y;
    unsigned int d = (unsigned int)a - nlen;
    buf[0] = 0;
    a = (unsigned char)d;
    if (!a) { leftz(s, 0xFF); return; }
    if (d & 0x8000) {                       /* longer than the format: cut */
        a = (unsigned char)(0 - a) >> 1;
        leftz(s, (unsigned char)(a - 1));
        return;
    }
    bl = a >> 1;
    if (!bl) { leftz(s, 0xFF); return; }
    y = 0xFF;
    do {
        do y++; while (buf[y] != '#');
        buf[y] = ' ';
    } while (--bl);
    leftz(s, 0xFF);
}

static void arg(void) {
    unsigned char y = 0xFF, c;
    fix = frac = 0;
    dot = 0xFF;
    if (PU_FLAG & 0x10) { _float_str[0] = '0'; _float_str[1] = 0; }
    for (;;) {
        c = _float_str[++y];
        if (!c) break;
        if (c == ' ') continue;
        if (c == '.') { flag |= 0x40; dot = y; continue; }
        if (flag & 0x40) frac++; else fix++;
    }
    nlen = y;
    if (dot == 0xFF) dot = y;
}

static void round_(void) {
    unsigned char x, c;
    if (after >= frac) return;
    x = (unsigned char)(dot + after + 1);
    c = _float_str[x];
    if (c < '5' || c > '9') return;
    for (;;) {
        x--;
        if (!x) return;
        c = _float_str[x];
        if (c < '0') continue;
        c++;
        if (c <= '9') { _float_str[x] = c; return; }
        _float_str[x] = '0';
    }
}

static void insert(void) {
    unsigned char x, y, fillc;
    if (before < fix) {                     /* no fit: *s */
        for (y = 0; y < hulp; y++)
            if (buf[y] == '#') buf[y] = '*';
        return;
    }
    x = dot;
    y = point;
    for (;;) {
        x--;
        if (x & 0x80) break;
        do {
            y--;
            if (y & 0x80) goto after_;
        } while (buf[y] != '#');
        buf[y] = _float_str[x];
        if (buf[y] == '-') flag &= ~8;
        else if (buf[y] == ' ' && (flag & 8)) buf[y] = '0';
    }
    fillc = (flag & 8) ? '0' : ' ';
    for (;;) {
        y--;
        if (y & 0x80) break;
        if (buf[y] == '#') buf[y] = fillc;
    }
after_:
    if (!after) return;
    x = dot;
    y = point;
    for (;;) {
        x++;
        if (x >= nlen) break;
        do {
            y++;
            if (y >= hulp) return;
        } while (buf[y] != '#');
        if (!_float_str[x]) break;
        buf[y] = _float_str[x];
    }
    for (;;) {
        y++;
        if (y >= hulp) return;
        if (buf[y] == '#') buf[y] = '0';
    }
}

/* %wX: floor(value) as 32 bits, hexadecimal, into the #s left of the point */
static void hex(void) {
    unsigned long v = 0, m;
    unsigned char k, x = 8, y = point, d[8], e = _float_a[0];
    /* (facint: the floor, from the float's bytes: exponent, sign and
     * mantissa) */
    if (e > 128) {
        unsigned char sh = (unsigned char)(e - 128);
        m = (unsigned long)(_float_a[1] | 0x80) << 24 | (unsigned long)_float_a[2] << 16 |
            (unsigned int)_float_a[3] << 8 | _float_a[4];
        if (sh >= 32) v = m << (sh - 32 > 31 ? 0 : sh - 32);
        else {
            v = m >> (32 - sh);
            if ((_float_a[1] & 0x80) && (m << sh)) v++;     /* (negative with a fraction: down) */
        }
        if (_float_a[1] & 0x80) v = 0 - v;
    } else if (e && (_float_a[1] & 0x80)) v = 0xFFFFFFFFUL;  /* (-1 < x < 0: -1) */
    for (k = 8; k--;) {
        unsigned char n = (unsigned char)(v & 15);
        d[k] = n < 10 ? '0' + n : L_A + n - 10;
        v >>= 4;
    }
    for (;;) {
        x--;
        if (x & 0x80) return;
        do {
            y--;
            if (y & 0x80) return;
        } while (buf[y] != '#');
        buf[y] = d[x];
    }
}

void _bas_using(void) {
    unsigned char y;
    fmt = _bas_sa + 1;
    fmtlen = _bas_sa[0];
    if (!fmtlen) { _bas_n = 11; _bas_error(); }
    PU_FLAG = 0;
    parse();
    if (_bas_m) {
        nlen = _bas_sb[0];
        if (buf[0] == '>') right(_bas_sb + 1);
        else if (buf[0] == '=') centre(_bas_sb + 1);
        else leftz(_bas_sb + 1, 0xFF);
    } else if (type == L_X) hex();
    else {
        _float_fout();
        arg();
        round_();
        insert();
    }
    for (y = 0; y < hulp; y++)
        if (buf[y]) putc_(buf[y]);
}
