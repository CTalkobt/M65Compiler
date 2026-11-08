/* basic_gfx.c — the graphics statements of BASIC 65 programs compiled by
 * basic45 --compile (#369): SCREEN (DEF, OPEN, SET, CLOSE, CLR, the old
 * SCREEN [n,] width, height, depth), PEN, DMODE, DPAT, DOT, LINE, BOX,
 * PALETTE of a screen, SCNCLR colour; RGRAPHIC, PIXEL, RPEN, RPALETTE.
 *
 * As BASIC 65's graphics kernel (b65alt.src V920413, kg65.*), written here
 * (the kernel's code not called): the VIC-III's bitplanes in 8 KB segments of
 * banks 4 (even planes) and 5 (odd), each screen's palette in its segments'
 * spare bytes. The state is the kernel's own variables (bank 0, $1F00: the
 * screen table at $1F2C, the pens, the draw and view screens), so the
 * interpreter's kernel and a compiled program agree on what is open.
 *
 * Arguments: _bas_g[] (the kernel's GKI.parm1-18 order: values as BASIC
 * parsed them), __bas_n / __bas_m for the functions' results.
 */

extern int _bas_n, _bas_m;
extern void _bas_error(void);
extern void _bas_out(void);

unsigned char _bas_g[20];        /* the statement's parameters (GKI.parm1..) */

#define VIC ((volatile unsigned char *)0xD000)
#define RED ((volatile unsigned char *)0xD100)
#define GREEN ((volatile unsigned char *)0xD200)
#define BLUE ((volatile unsigned char *)0xD300)
#define CRMODE (*(volatile unsigned char __far *)0xD7UL)

/* The kernel's variables (kg65vars.src), at $1F00 */
struct kg65 {
    unsigned char parm[17];          /* $00 */
    unsigned char subparm[5];        /* $11 */
    unsigned char temp[9];           /* $16 */
    unsigned char bpa[4], bpb[4];    /* $1F */
    unsigned int vpwx, vphy;         /* $27 */
    unsigned char scrtabidx;         /* $2B */
    unsigned char scrtab[16];        /* $2C: mode, planes, bank A, bank B segments */
    unsigned char bankablks, bankbblks, banka, bankb;    /* $3C */
    unsigned char drawscrbyte;       /* $40 */
    unsigned char drawpens[3];       /* $41 */
    unsigned char drawmode;          /* $44: 0 jam2, 1 complement, 2 stencil, 3-4 style, 5-7 thickness */
    unsigned char dpatype;           /* $45 */
    unsigned char dpattern[4];       /* $46 */
    unsigned char patrbitnum;        /* $4A */
    unsigned char drawscreen;        /* $4B: index 0, 4, 8, 12; $FF none */
    unsigned char drawscrwidth, drawscrheight, drawscrdepth, drawscrcols;
    unsigned char drawscrbp, drawscrablks, drawscrbblks;
    int vpminx, vpminy;              /* $53 */
    unsigned int vpmaxx, vpmaxy;     /* $57 */
    unsigned int drawscrinc;         /* $5B */
    unsigned char scanlineflag, controb, free5f[2];  /* $5D */
    unsigned char dsbp0adr[8];       /* $61: plane segment addresses (nybbles) */
    unsigned char viewscreen, viewscrwidth, viewscrheight, viewscrdepth;    /* $69 */
};
#define KG (*(struct kg65 *)0x1F00)

static const unsigned char bitsdn[8] = {128, 64, 32, 16, 8, 4, 2, 1};
static const unsigned char twosdn[4] = {0xC0, 0x30, 0x0C, 0x03};
static const unsigned char patrbits[4] = {8, 16, 24, 32};
static const unsigned char dpatterns[4] = {0x88, 0xCC, 0xEE, 0x9C};

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

static void putc_(unsigned char c) { __jsr_raw(_bas_out, .A = c); }

/* ?NO GRAPHICS AREA without a draw screen (BASIC's CheckGraphicMode) */
void _bas_gcheck(void) {
    if (KG.drawscreen & 0x80) raise(35);
}

/* GRAPHIC CLR: the kernel's variables cleared, no draw or view screen, the
 * banks' free segments the KERNAL's masks (kg65.start) */
void _bas_ginit(void) {
    unsigned char k = 0;
    do ((unsigned char *)&KG)[k] = 0; while (++k);
    VIC[0x3B] = 0;
    KG.drawscreen = KG.viewscreen = 0xFF;
    KG.bankablks = *(volatile unsigned char *)0x02AB;
    KG.bankbblks = *(volatile unsigned char *)0x02AC;
    KG.banka = 4;
    KG.bankb = 5;
    VIC[0x2F] = 0x47;
    VIC[0x2F] = 0x53;
    VIC[0x7C] = (unsigned char)((VIC[0x7C] & 0xF8) | 2);
}

/* --- screens --- */

/* the screen table entry of screen y: width, height, depth, open (xpdscrdata);
 * 0 when not defined */
static unsigned char entry(unsigned char y) {
    unsigned char m = KG.scrtab[y << 2];
    KG.scrtabidx = (unsigned char)(y << 2);
    return (m & 0x80) ? m : 0;
}

void _bas_gscrdef(void) {
    /* parm: screen, width, height, depth (0-7) */
    KG.scrtab[_bas_g[0] << 2] = (unsigned char)(0x80 | (_bas_g[1] << 4) | (_bas_g[2] << 3) | _bas_g[3]);
}

/* a free segment of bank x (0 A, 1 B): the lowest free bit, a pair of bits
 * for a 640 wide screen; its bit(s) marked used and added to *mask */
static unsigned char allocseg(unsigned char x, unsigned char wide, unsigned char *mask) {
    unsigned char used = x ? KG.bankbblks : KG.bankablks, b = 0, k;
    if (!wide) {
        for (k = 0; k < 8; k++)
            if (!(used & (1 << k))) { b = (unsigned char)(1 << k); break; }
    } else {
        for (k = 0; k < 4; k++)
            if (!(used & (3 << (k * 2)))) { b = twosdn[3 - k]; break; }
    }
    if (!b) return 1;
    if (x) KG.bankbblks |= b;
    else KG.bankablks |= b;
    *mask |= b;
    return 0;
}

/* SCREEN OPEN: 1 when it cannot (not defined, open already, no memory) */
static unsigned char open_(unsigned char y) {
    unsigned char m = entry(y), planes = 0, ma = 0, mb = 0, d, wide, tall;
    if (!m || (m & 0x40)) return 1;
    wide = m & 0x10;
    tall = m & 0x08;
    for (d = 0; d <= (m & 7); d++) {
        unsigned char x = d & 1;
        planes |= (unsigned char)(1 << d);
        if (allocseg(x, wide, x ? &mb : &ma)) return 1;
        if (tall && allocseg(x, wide, x ? &mb : &ma)) return 1;
    }
    y <<= 2;
    KG.scrtab[y] = m | 0x40;
    KG.scrtab[y + 1] = planes;
    KG.scrtab[y + 2] = ma;
    KG.scrtab[y + 3] = mb;
    return 0;
}

void _bas_gscropen(void) {
    if (open_(_bas_g[0])) raise(16);
}

/* the planes' segment addresses into r[8] (set_bp_register): the next used
 * segment of the plane's bank for each enabled plane from 7 down; 640 takes a
 * second, 400 an odd-line one (high nybble); 1 when the masks run out */
static unsigned char bp_regs(unsigned char idx, volatile unsigned char *r) {
    unsigned char mode = KG.scrtab[idx], planes = KG.scrtab[idx + 1];
    unsigned char mask[2], seg[2];
    signed char y;
    mask[0] = KG.scrtab[idx + 2];
    mask[1] = KG.scrtab[idx + 3];
    seg[0] = seg[1] = 0;
    for (y = 7; y >= 0; y--) {
        unsigned char z = (unsigned char)(y & 1), s, i;
        if (!(planes & (1 << y))) continue;
        for (i = 0; i < ((mode & 0x08) ? 2 : 1); i++) {
            unsigned char k;
            /* next segment: a set bit of the mask, from bit 0, counted */
            for (;;) {
                s = seg[z]++;
                k = mask[z] & 1;
                mask[z] >>= 1;
                if (k) break;
                if (!mask[z]) return 1;
            }
            s <<= 1;
            if (i == 0) r[y] = s;
            else r[y] = (unsigned char)(r[y] | (s << 4));
            if (mode & 0x10) {
                for (;;) {
                    k = mask[z] & 1;
                    seg[z]++;
                    mask[z] >>= 1;
                    if (k) break;
                    if (!mask[z]) return 1;
                }
            }
        }
    }
    return 0;
}

/* screen y's colour c as a colour index: bit k of c on the k-th plane it uses */
static unsigned char color2index(unsigned char idx, unsigned char c) {
    unsigned char planes = KG.scrtab[idx + 1], r = 0, bit = 1, p = 1;
    for (;;) {
        while (!(planes & p)) {
            p <<= 1;
            if (!p) return r;
        }
        if (c & bit) r |= p;
        p <<= 1;
        bit <<= 1;
        if (!bit || !p) return r;
    }
}

/* where screen y's colour c is in its palette area: its far address, the
 * byte pair's index (rw: 0 read, 1 write); 0: not open */
static unsigned long palette_at(unsigned char y, unsigned char c, unsigned char *z) {
    unsigned char idx = (unsigned char)(y << 2), m = KG.scrtab[idx], bank, plane = 0, bit = 1, mask;
    unsigned int adr;
    unsigned char k, two = (m & 0x10) || (c & 0x80);
    if (!(m & 0x40)) return 0;
    for (k = 0; k < 2; k++) {
        mask = KG.scrtab[idx + 2 + k];
        plane = 0;
        bit = 1;
        while (bit && !(mask & bit)) { plane++; bit <<= 1; }
        if (!bit) continue;
        if (two) {
            plane++;
            bit <<= 1;
            while (bit && !(mask & bit)) { plane++; bit <<= 1; }
            if (!bit) continue;
        }
        break;
    }
    if (k == 2) return 0;
    bank = k ? KG.bankb : KG.banka;
    if (!(m & 0x10)) adr = (unsigned int)(((plane << 5) | 0x1F) << 8) | 0x40;      /* 8000 */
    else {
        /* 16000: the second segment's number through lsr, ror, ror as the
         * kernel's (which puts segments 0-1's at $7E80) */
        unsigned char a = plane, c = a & 1, nc;
        a >>= 1;
        nc = a & 1;
        a = (unsigned char)((c << 7) | (a >> 1));
        c = nc;
        a = (unsigned char)((c << 7) | (a >> 1));
        adr = (unsigned int)((a | 0x3E) << 8) | 0x80;
    }
    if (c & 0x80) {
        c &= 0x7F;
        if (m & 0x10) adr += 192;
    }
    *z = (unsigned char)((c >> 1) + c);
    return ((unsigned long)bank << 16) | adr;
}

/* screen y's colour c: r, g, b from / to its palette (3 bytes for 2 colours);
 * 1 when not open */
static unsigned char palette(unsigned char y, unsigned char c, unsigned char *rgb, unsigned char wr) {
    unsigned char z;
    unsigned long a = palette_at(y, c, &z);
    unsigned char __far *p;
    if (!a) return 1;
    p = (unsigned char __far *)(a + z);
    if (!(c & 1)) {
        if (wr) {
            p[0] = (unsigned char)((rgb[0] << 4) | rgb[1]);
            p[1] = (unsigned char)((p[1] & 0x0F) | (rgb[2] << 4));
        } else {
            rgb[0] = p[0] >> 4;
            rgb[1] = p[0] & 15;
            rgb[2] = p[1] >> 4;
        }
    } else {
        if (wr) {
            p[0] = (unsigned char)((p[0] & 0xF0) | rgb[0]);
            p[1] = (unsigned char)((rgb[1] << 4) | rgb[2]);
        } else {
            rgb[0] = p[0] & 15;
            rgb[1] = p[1] >> 4;
            rgb[2] = p[1] & 15;
        }
    }
    return 0;
}

/* back to the text screen when no screen is viewed (CheckViewScreen) */
static void textview(void) {
    if (!(KG.viewscreen & 0x80)) return;
    VIC[0x31] &= 0xEF;
    putc_(27);
    putc_((CRMODE & 0x40) ? 0x35 : (CRMODE & 0x80) ? 0x34 : 0x38);
}

/* SCREEN SET draw, view (255: draw as it is, view text): 1 for an error */
static unsigned char set_(unsigned char d, unsigned char v) {
    unsigned char m, idx, k;
    if (!(d & 0x80)) {
        if (d >= 4) return 1;
        m = entry(d);
        if (!m || !(m & 0x40)) return 1;
        idx = (unsigned char)(d << 2);
        KG.drawscreen = idx;
        KG.drawscrwidth = (m >> 4) & 1;
        KG.drawscrheight = (m >> 3) & 1;
        KG.drawscrdepth = m & 7;
        KG.drawscrcols = KG.drawscrwidth ? 80 : 40;
        KG.vpwx = KG.vpmaxx = KG.drawscrwidth ? 640 : 320;
        KG.drawscrinc = KG.vpmaxx - 8;
        KG.vphy = KG.vpmaxy = KG.drawscrheight ? 400 : 200;
        KG.drawscrbp = KG.scrtab[idx + 1];
        KG.drawscrablks = KG.scrtab[idx + 2];
        KG.drawscrbblks = KG.scrtab[idx + 3];
        KG.vpminx = KG.vpminy = 0;
        if (bp_regs(idx, KG.dsbp0adr)) return 1;
    }
    if (v & 0x80) {
        KG.viewscreen = v;
        textview();
        return 0;
    }
    if (v >= 4) return 1;
    m = entry(v);
    if (!m || !(m & 0x40)) return 1;
    idx = (unsigned char)(v << 2);
    KG.viewscreen = idx;
    KG.viewscrwidth = (m >> 4) & 1;
    KG.viewscrheight = (m >> 3) & 1;
    KG.viewscrdepth = m & 7;
    /* the screen's palette into the hardware's */
    {
        unsigned int n = (unsigned int)(2u << KG.viewscrdepth), c;
        for (c = 0; c < n; c++) {
            unsigned char rgb[3], i;
            palette(v, (unsigned char)c, rgb, 0);
            i = color2index(idx, (unsigned char)c);
            RED[i] = rgb[0];
            GREEN[i] = rgb[1];
            BLUE[i] = rgb[2];
        }
    }
    KG.controb = VIC[0x31];
    k = (unsigned char)((VIC[0x31] & 0x60) | 0x10);
    if (KG.viewscrwidth) k |= 0x80;
    if (KG.viewscrheight) k |= 0x09;
    if (bp_regs(idx, VIC + 0x33)) return 1;
    VIC[0x31] = k;
    VIC[0x16] = (k & 0x80) ? 0x29 : 0x28;
    VIC[0x32] = KG.scrtab[idx + 1];
    if (CRMODE & 0x80)
        for (k = 0; k < 8; k++) ((volatile unsigned char *)0x0FF8)[k] = ((volatile unsigned char *)0x0BF8)[k];
    VIC[0x6D] = 0x0F;
    return 0;
}

void _bas_gscrset(void) {
    if (set_(_bas_g[0], _bas_g[1])) raise(35);
}

/* SCREEN CLOSE: the segments freed; the draw screen becomes the view screen,
 * the view one text (deallocscr) */
static unsigned char close_(unsigned char y) {
    unsigned char idx = (unsigned char)(y << 2), m = KG.scrtab[idx], k;
    KG.scrtabidx = idx;
    if (!(m & 0x80) || !(m & 0x40)) return 1;
    KG.bankablks &= (unsigned char)~KG.scrtab[idx + 2];
    KG.bankbblks &= (unsigned char)~KG.scrtab[idx + 3];
    KG.scrtab[idx] = m & 0xBF;
    if (idx == KG.drawscreen) {
        KG.drawscreen = KG.viewscreen;
        KG.drawscrwidth = KG.viewscrwidth;
        KG.drawscrheight = KG.viewscrheight;
        KG.drawscrdepth = KG.viewscrdepth;
    }
    if (idx != KG.viewscreen) return 0;
    if (KG.viewscreen == KG.drawscreen) {
        unsigned char *d = &KG.drawscreen;
        for (k = 0; k < 8; k++) {
            d[k] = 0;
            KG.dsbp0adr[k] = 0;
            ((unsigned char *)&KG.vpminx)[k] = 0;
        }
        KG.drawscreen = 0xFF;
    }
    VIC[0x32] = 0;
    for (k = 0; k < 8; k++) VIC[0x33 + k] = 0;
    KG.viewscreen = KG.viewscrwidth = KG.viewscrheight = KG.viewscrdepth = 0;
    KG.viewscreen = 0xFF;
    textview();
    return 0;
}

void _bas_gscrclose(void) {
    if (!close_(_bas_g[0])) textview();     /* (the kernel falls through to CheckViewScreen) */
    __jsr(0xE027);                   /* _palette_init */
}

/* DMA fill of a segment (8000 bytes, 16000 for 640) */
static unsigned char dmalist[12];
static void dmafill(unsigned char bank, unsigned char hi, unsigned char v) {
    unsigned int n = KG.drawscrwidth ? 16000 : 8000;
    dmalist[0] = 3;
    dmalist[1] = (unsigned char)n;
    dmalist[2] = (unsigned char)(n >> 8);
    dmalist[3] = v;
    dmalist[4] = dmalist[5] = 0;
    dmalist[6] = 0;
    dmalist[7] = hi;
    dmalist[8] = bank;
    dmalist[9] = dmalist[10] = dmalist[11] = 0;
    VIC[0x702] = 0;                  /* ($D702 bank, $D701 high, $D700 low & go) */
    VIC[0x701] = (unsigned char)((unsigned int)dmalist >> 8);
    VIC[0x700] = (unsigned char)(unsigned int)dmalist;
}

/* SCREEN CLR / SCNCLR colour (screenclear) */
void _bas_gclear(void) {
    unsigned char fill[16], idx = KG.drawscreen, c, x, b;
    if (idx & 0x80) raise(35);
    c = color2index(idx, _bas_g[0]);
    for (x = 0; x < 16; x++) fill[x] = 0;   /* (the kernel's are the line A area's bytes) */
    for (x = 0; x < 8; x++) {
        unsigned char v, s, o = (x & 1) ? 8 : 0;
        if (!(KG.drawscrbp & (1 << x))) continue;
        v = (c & (1 << x)) ? 0xFF : 0;
        s = (KG.dsbp0adr[x] >> 1) & 7;
        fill[o + s] = v;
        if (KG.drawscrwidth) fill[o + ((s + 1) & 7)] = v;
        if (KG.drawscrheight) {
            s = (KG.dsbp0adr[x] >> 5) & 7;
            fill[o + s] = v;
            if (KG.drawscrwidth) fill[o + ((s + 1) & 7)] = v;
        }
    }
    for (b = 0; b < 2; b++) {
        unsigned char used = KG.scrtab[idx + 2 + b], hi = 0;
        unsigned char base = b ? 8 : 0;
        for (x = 0; x < 8; x += 2) {
            if (used & 1) dmafill(b ? KG.bankb : KG.banka, hi, fill[base + x]);
            used >>= 1;
            hi += 0x20;
            if ((used & 1) && !KG.drawscrwidth) dmafill(b ? KG.bankb : KG.banka, hi, fill[base + x + 1]);
            used >>= 1;
            hi += 0x20;
        }
    }
}

/* the KERNAL's default palette into screen y: colours 0 .. min(31, 2^depth-1) */
static void default_palette(unsigned char y, unsigned char depth) {
    signed int c = depth < 4 ? (1 << (depth + 1)) - 1 : 31;
    __jsr(0xE027);                   /* _palette_init */
    for (; c >= 0; c--) {
        unsigned char rgb[3];
        rgb[0] = RED[c];
        rgb[1] = GREEN[c];
        rgb[2] = BLUE[c];
        palette(y, (unsigned char)c, rgb, 1);
        if ((unsigned char)(y << 2) == KG.viewscreen) {
            unsigned char i = color2index(KG.viewscreen, (unsigned char)c);
            RED[i] = rgb[0];
            GREEN[i] = rgb[1];
            BLUE[i] = rgb[2];
        }
    }
}

/* SCREEN [n,] width, height, depth: defined, opened, drawn and viewed,
 * cleared, the default palette, pen 0 white */
void _bas_gscreen(void) {
    unsigned char y = _bas_g[0];
    _bas_gscrdef();
    if (open_(y)) raise(16);
    set_(y, y);
    _bas_g[0] = 0;
    _bas_gclear();
    default_palette(y, _bas_g[3]);
    KG.drawpens[0] = color2index(KG.drawscreen, 1);
}

/* PEN [n,] colour: pen n (0-2) a colour index of the draw screen */
void _bas_gpen(void) {
    KG.drawpens[_bas_g[0]] = color2index(KG.drawscreen, _bas_g[1]);
}

/* DMODE jam, complement, stencil, style, thickness */
void _bas_gdmode(void) {
    KG.drawmode = (unsigned char)(_bas_g[0] | (_bas_g[1] << 1) | (_bas_g[2] << 2) | ((_bas_g[3] & 3) << 3) |
                                  ((_bas_g[4] & 7) << 5));
    KG.patrbitnum = 0;
}

/* DPAT type [, n, b1 .. bn] */
void _bas_gdpat(void) {
    unsigned char t = _bas_g[0], k;
    for (k = 0; k < 4; k++) KG.dpattern[k] = 0;
    KG.patrbitnum = 0;
    KG.dpatype = (unsigned char)(t << 2);
    if (t) {
        KG.dpattern[0] = dpatterns[t - 1];
        return;
    }
    k = (unsigned char)((_bas_g[1] - 1) & 3);
    KG.dpatype |= k;
    for (;;) {
        KG.dpattern[k] = _bas_g[2 + k];
        if (!k) break;
        k--;
    }
}

/* PALETTE screen, colour, red, green, blue */
void _bas_gpal(void) {
    unsigned char y = _bas_g[0], c = _bas_g[1], rgb[3];
    rgb[0] = _bas_g[2];
    rgb[1] = _bas_g[3];
    rgb[2] = _bas_g[4];
    if (palette(y, c, rgb, 1)) raise(35);
    if ((unsigned char)(y << 2) == KG.viewscreen) {
        unsigned char i = color2index(KG.viewscreen, c);
        RED[i] = rgb[0];
        GREEN[i] = rgb[1];
        BLUE[i] = rgb[2];
    }
}

/* --- drawing --- */

static int lx0, ly0, lx1, ly1;   /* the line's ends (the kernel's linex0 ..) */

/* the dot at (x, y) of the draw screen with the pens, the mode and the
 * pattern (plotdot: basic_gfxa.s45) */
extern unsigned int _bas_gx, _bas_gy;
extern void _bas_gplot(void);
static void plot(unsigned int x, unsigned int y) {
    _bas_gx = x;
    _bas_gy = y;
    _bas_gplot();
}

/* the clip's intersection: n * d1 / d2 as BASIC's floats, rounded down (calcxypt) */
extern unsigned char _float_a[5];
extern void _bas_ayint(void);
static int calcxypt(int n, int d1, int d2) {
    float f = (float)n * (float)d1;
    unsigned char k;
    if (d2) f = f / (float)d2;
    for (k = 0; k < 5; k++) _float_a[k] = ((unsigned char *)&f)[k];
    _bas_ayint();
    return _bas_n;
}

/* Cohen-Sutherland against the viewport (kg65.clip): 1 when it is all outside */
static unsigned char outcode(int x, int y, int maxx, int maxy) {
    unsigned char c = 0;
    if (y < KG.vpminy) c |= 2;
    else if (y > maxy) c |= 8;
    if (x < KG.vpminx) c |= 1;
    else if (x > maxx) c |= 4;
    return c;
}

static unsigned char clip(void) {
    int maxx = (int)KG.vpmaxx - 1, maxy = (int)KG.vpmaxy - 1;
    for (;;) {
        unsigned char c0 = outcode(lx0, ly0, maxx, maxy), c1 = outcode(lx1, ly1, maxx, maxy), c;
        int dx = lx1 - lx0, dy = ly1 - ly0, x, y;
        int *px, *py;
        if (c0 & c1) return 1;
        if (!c0 && !c1) return 0;
        /* the point outside: the first, else the second (its own deltas from the first) */
        if (c0) { c = c0; px = &lx0; py = &ly0; }
        else { c = c1; px = &lx1; py = &ly1; }
        x = *px;
        y = *py;
        /* (the distances to the low edges one more, as calconst's clc / sbc) */
        if (c & 8) { x = calcxypt(maxy - ly0, dx, dy) + lx0; y = maxy; }
        else if (c & 2) { x = calcxypt(KG.vpminy - ly0 - 1, dx, dy) + lx0; y = KG.vpminy; }
        else if (c & 4) { y = calcxypt(maxx - lx0, dy, dx) + ly0; x = maxx; }
        else if (c & 1) { y = calcxypt(KG.vpminx - lx0 - 1, dy, dx) + ly0; x = KG.vpminx; }
        *px = x;
        *py = y;
    }
}

/* a line from (lx0, ly0) to (lx1, ly1), clipped, each dot plotted (q.line) */
static void line_(void) {
    unsigned char oct = 0;
    int dx, dy, e;
    unsigned int n, len;
    if (clip()) return;
    dx = lx1 - lx0;
    if (dx < 0) { oct |= 2; dx = -dx; }
    dy = ly1 - ly0;
    if (dy < 0) { oct |= 4; dy = -dy; }
    if (dx - dy < 0) {
        e = dx; dx = dy; dy = e;
        oct |= 8;
    }
    e = (-dx) >> 1;
    len = (unsigned int)dx;
    plot((unsigned int)lx0, (unsigned int)ly0);
    for (n = 0; n < len; n++) {
        unsigned char o = oct, xs, ys;
        e += dy;
        if (e >= 0) {
            e -= dx;
            o++;
        }
        /* octable: x step, y step */
        if (o & 8) { ys = 1; xs = o & 1; }
        else { xs = 1; ys = o & 1; }
        if (xs) lx0 += (o & 2) ? -1 : 1;
        if (ys) ly0 += (o & 4) ? -1 : 1;
        plot((unsigned int)lx0, (unsigned int)ly0);
    }
}

/* DOT x, y [, colour] (colour: pen 0 becomes it, as the kernel's) */
void _bas_gdot(void) {
    unsigned int x = *(unsigned int *)&_bas_g[2], y = *(unsigned int *)&_bas_g[4];
    if ((x | y) & 0x8000) return;
    if (x >= KG.vpmaxx || y >= KG.vpmaxy || x < (unsigned int)KG.vpminx || y < (unsigned int)KG.vpminy) return;
    if (!(_bas_g[0] & 0x80)) KG.drawpens[0] = color2index(KG.drawscreen, _bas_g[1]);
    plot(x, y);
}

/* LINE: the first point (_bas_g 0-3) also the end's default (4-7) */
void _bas_glstart(void) {
    unsigned char k;
    for (k = 0; k < 4; k++) _bas_g[4 + k] = _bas_g[k];
}

/* LINE: from (_bas_g 0-3) to (4-7); the end is the next line's start */
void _bas_gline(void) {
    unsigned char k;
    lx0 = *(int *)&_bas_g[0];
    ly0 = *(int *)&_bas_g[2];
    lx1 = *(int *)&_bas_g[4];
    ly1 = *(int *)&_bas_g[6];
    line_();
    for (k = 0; k < 4; k++) _bas_g[k] = _bas_g[4 + k];
}

/* the next dot of an edge (kg65.s.line with doadr: unclipped), as the solid
 * box's two edges step */
struct edge { int x, y, x1, y1, dx, dy, e; unsigned char oct, first; unsigned int n; };
static unsigned char step(struct edge *g) {
    if (g->first) {
        g->first = 0;
        g->oct = 0;
        g->dx = g->x1 - g->x;
        if (g->dx < 0) { g->oct |= 2; g->dx = -g->dx; }
        g->dy = g->y1 - g->y;
        if (g->dy < 0) { g->oct |= 4; g->dy = -g->dy; }
        if (g->dx - g->dy < 0) {
            int t = g->dx; g->dx = g->dy; g->dy = t;
            g->oct |= 8;
        }
        g->e = (-g->dx) >> 1;
        g->n = 0xFFFF;
        return 0;
    }
    {
        unsigned char o = g->oct;
        g->e += g->dy;
        if (g->e >= 0) { g->e -= g->dx; o++; }
        if (o & 8) {
            if (o & 1) g->x += (o & 2) ? -1 : 1;
            g->y += (o & 4) ? -1 : 1;
        } else {
            g->x += (o & 2) ? -1 : 1;
            if (o & 1) g->y += (o & 4) ? -1 : 1;
        }
    }
    g->n++;
    return g->n >= (unsigned int)g->dx;
}

/* BOX: _bas_g[0] the values' count * 2, then the values (words) */
void _bas_gbox(void) {
    unsigned int *v = (unsigned int *)&_bas_g[1];    /* GKI.parm2.. */
    unsigned char count = (unsigned char)(_bas_g[0] | 2), solid, k;
    int cx[4], cy[4];
    solid = _bas_g[count - 1];                       /* (parm1 + count - 1: the flag's low byte) */
    if (count == 18) {
        for (k = 0; k < 4; k++) { cx[k] = (int)v[k * 2]; cy[k] = (int)v[k * 2 + 1]; }
    } else if (count == 10) {
        cx[0] = (int)v[0]; cy[0] = (int)v[1];
        cx[1] = (int)v[2]; cy[1] = (int)v[1];
        cx[2] = (int)v[2]; cy[2] = (int)v[3];
        cx[3] = (int)v[0]; cy[3] = (int)v[3];
    } else {
        /* (fewer values: nothing, the kernel's carry clear; between: ?SYNTAX ERROR) */
        if (count - 1 >= 9) raise(11);
        return;
    }
    if (solid) {
        struct edge a, b;
        a.x = cx[0]; a.y = cy[0]; a.x1 = cx[3]; a.y1 = cy[3]; a.first = 1;
        b.x = cx[1]; b.y = cy[1]; b.x1 = cx[2]; b.y1 = cy[2]; b.first = 1;
        for (;;) {
            if (step(&a)) return;
            KG.patrbitnum = 0;
            step(&b);
            lx0 = a.x; ly0 = a.y; lx1 = b.x; ly1 = b.y;
            line_();
        }
    }
    KG.patrbitnum = 0;
    for (k = 0; k < 4; k++) {
        lx0 = cx[k]; ly0 = cy[k];
        lx1 = cx[(k + 1) & 3]; ly1 = cy[(k + 1) & 3];
        line_();
    }
}

/* --- functions --- */

/* RGRAPHIC(screen, n) into __bas_n */
void _bas_grgraphic(void) {
    unsigned char y = _bas_g[0], n = _bas_g[1], m, r, k;
    unsigned char p[9];
    if (y >= 4 || n >= 11) raise(14);
    for (k = 0; k < 4; k++) p[k] = KG.scrtab[(y << 2) + k];
    p[4] = p[5] = 0xFF;
    for (k = 0; k < 4; k++) {
        if (KG.drawscreen == (unsigned char)(k << 2)) p[4] = k;
        if (KG.viewscreen == (unsigned char)(k << 2)) p[5] = k;
    }
    p[6] = KG.drawmode;
    p[7] = KG.dpatype;
    m = p[0];
    if (n == 0) r = (unsigned char)(((m ^ 0x80) >> 6) & 3);   /* 0 closed, 1 open, 2 not defined */
    else if (n == 1) r = (m >> 4) & 3;                       /* the width flag */
    else if (n == 2) r = (unsigned char)((m & 8) >> 3);      /* the height flag */
    else if (n == 3) r = (unsigned char)((m & 7) + 1);
    else r = p[n - 3];
    _bas_n = r;
}

/* PIXEL(x, y): the colour index there (0 outside the viewport's far edges) */
void _bas_gpixel(void) {
    unsigned int x = *(unsigned int *)&_bas_g[0], y = *(unsigned int *)&_bas_g[2];
    unsigned char odd = 0, line, mask, plane, bit = 1, r = 0;
    unsigned int adr;
    if (x >= KG.vpmaxx || y >= KG.vpmaxy) { _bas_n = 0; return; }
    if (KG.drawscrheight) { odd = (unsigned char)(y & 1); y >>= 1; }
    line = (unsigned char)y;
    adr = (unsigned int)(line >> 3) * 0x500;
    adr >>= KG.drawscrwidth ? 1 : 2;
    adr |= line & 7;
    adr += x & 0xFFF8;
    mask = bitsdn[x & 7];
    for (plane = 0; plane < 8; plane++, bit <<= 1) {
        unsigned char seg;
        unsigned char __far *p;
        if (!(KG.drawscrbp & bit)) continue;
        seg = KG.dsbp0adr[plane];
        seg = (KG.drawscrheight && odd) ? (seg & 0xE0) : (unsigned char)(seg << 4);
        p = (unsigned char __far *)(((unsigned long)(KG.banka | (plane & 1)) << 16) |
                                    (unsigned int)(((seg | (adr >> 8)) << 8) | (adr & 0xFF)));
        if (*p & mask) r |= bit;
    }
    _bas_n = r;
}

/* RPEN(n) */
void _bas_grpen(void) {
    _bas_n = KG.drawpens[_bas_g[0]];
}

/* RPALETTE(screen, colour, 0-2) */
void _bas_grpalette(void) {
    unsigned char rgb[3];
    if (_bas_g[0] >= 4 || palette(_bas_g[0], _bas_g[1], rgb, 0) || _bas_g[2] >= 3) raise(14);
    _bas_n = rgb[_bas_g[2]];
}

/* the text screen viewed again (an error, BREAK: RestoreTextScreen) */
void _bas_gtext(void) {
    if (KG.drawscreen & 0x80) return;
    set_(0xFF, 0xFF);
}
