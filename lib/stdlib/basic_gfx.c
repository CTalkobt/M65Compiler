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

#include <cbm.h>

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
extern struct kg65 _bas_kg;     /* (basic_gfxa.s45: the symbol at $1F00, so cc45 addresses it absolutely) */
#define KG _bas_kg

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

#define PLENUM (*(volatile unsigned char *)0x1176)   /* BASIC's plenum: the last screen's depth */
void _bas_gscrdef(void) {
    /* parm: screen, width, height, depth (0-7) */
    PLENUM = _bas_g[3];
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

/* the line's ends (the kernel's linex0 ..: basic_gfxa.s45's) */
extern int _bas_lx0, _bas_ly0, _bas_lx1, _bas_ly1;
#define lx0 _bas_lx0
#define ly0 _bas_ly0
#define lx1 _bas_lx1
#define ly1 _bas_ly1
extern void _bas_gline0(void);

/* the dot at (x, y) of the draw screen with the pens, the mode and the
 * pattern (plotdot: basic_gfxa.s45) */
extern unsigned int _bas_gx, _bas_gy;
extern void _bas_gplot(void);
static void plot(unsigned int x, unsigned int y) {
    _bas_gx = x;
    _bas_gy = y;
    _bas_gplot();
}

/* a line from (lx0, ly0) to (lx1, ly1), clipped, each dot plotted (q.line:
 * basic_gfxa.s45) */
static void line_(void) {
    _bas_gline0();
}

extern unsigned char _float_a[5];
extern void _bas_ayint(void);

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

/* --- ellipses (kg65.ellipse, epstab, cdot, octa / octb, solide: basic_gfxa.s45) --- */

static float fget(void) {
    float f;
    unsigned char k;
    for (k = 0; k < 5; k++) ((unsigned char *)&f)[k] = _float_a[k];
    return f;
}
static void fset(float f) {
    unsigned char k;
    for (k = 0; k < 5; k++) _float_a[k] = ((unsigned char *)&f)[k];
}

/* --- polygons (BASIC's kg65.polygon, the kernel's s.facet) --- */

extern void _float_cos(void);
extern void _float_sin(void);

/* the solid triangle (s.facet: basic_gfxa.s45) */
extern int _bas_fpx[3], _bas_fpy[3];
extern void _bas_gfacet(void);
static void facet(int *px, int *py) {
    unsigned char k;
    for (k = 0; k < 3; k++) { _bas_fpx[k] = px[k]; _bas_fpy[k] = py[k]; }
    _bas_gfacet();
}
/* POLYGON cx, cy, xr, yr, sides [, drawn sides, subtend, angle, solid] */
void _bas_gpolygon(void) {
    unsigned int *v = (unsigned int *)&_bas_g[1];
    int pcx = (int)v[0], pcy = (int)v[1], px0 = 0, py0 = 0, px1, py1, sx = 0, sy = 0;
    float tt, dg, xr = (float)(int)v[2], yr = (float)(int)v[3], c;
    unsigned char sides = (unsigned char)v[4], its, first, solid = (unsigned char)v[8];
    {
        float sa = (float)(int)v[7];
        tt = sa * 0.01745329252f;
    }
    if (sides) dg = 360.0f / (float)sides * 0.01745329252f;
    else dg = 0;
    its = (unsigned char)v[5] ? (unsigned char)v[5] : sides;
    first = !(its & 0x80);
    for (;;) {
        fset(tt);
        _float_cos();
        c = fget() * xr;
        fset(c);
        _bas_ayint();
        px1 = _bas_n + pcx;
        fset(tt);
        _float_sin();
        c = fget() * yr;
        fset(c);
        _bas_ayint();
        py1 = pcy - _bas_n;
        if (!first) {
            if (solid) {
                int fxs[3], fys[3];
                fxs[0] = px1; fys[0] = py1;     /* (fxmin, fxmid, fxmax: p1, the centre, p0) */
                fxs[1] = pcx; fys[1] = pcy;
                fxs[2] = px0; fys[2] = py0;
                px0 = px1; py0 = py1;
                facet(fxs, fys);
                goto next;
            }
            lx0 = px1; ly0 = py1; lx1 = px0; ly1 = py0;
            line_();
        }
        if (first) { sx = px1; sy = py1; }
        px0 = px1;
        py0 = py1;
    next:
        first = 0;
        if (its-- == 0) break;
        tt = tt + dg;
    }
    if ((unsigned char)v[6]) {
        lx0 = pcx; ly0 = pcy; lx1 = px0; ly1 = py0;
        line_();
        lx0 = pcx; ly0 = pcy; lx1 = sx; ly1 = sy;
        line_();
    }
}

void _bas_gpixel(void);

/* --- PAINT (kg65.paint, s.rfill, s.push: basic_gfxa.s45) --- */

extern void _bas_gfill(void);

/* PAINT x, y [, mode [, colour]] (GKI.parm: x, y, mode, colour) */
void _bas_gpaint(void) {
    if (_bas_g[5] == 1) _bas_g[7] = color2index(KG.drawscreen, _bas_g[7]);
    _bas_gfill();
}

/* --- VIEWPORT, GCOPY, CUT, PASTE (viewportdef, cut_copy, s.bltread, paste) --- */

#define BRUSH ((unsigned char *)0x1800)      /* the kernel's temp.brush (4 pages) */
#define SCANL ((unsigned char *)0x1E00)      /* its scanline buffer */
static const unsigned char maskl[8] = {0, 128, 192, 224, 240, 248, 252, 254};
static const unsigned char maskr[8] = {0, 127, 63, 31, 15, 7, 3, 1};
static unsigned int ax, ay, adx, ady, apos;
static unsigned char ab2sl, abor, abw, adepth, abrushw;
#define aodd KG.scanlineflag             /* (calcadr's odd line, the kernel's variable) */

/* VIEWPORT [DEF] x, y, width, height */
void _bas_gviewport(void) {
    unsigned int *v = (unsigned int *)&_bas_g[1];
    KG.vpminx = (int)v[0];
    KG.vpminy = (int)v[1];
    KG.vpwx = v[2];
    KG.vphy = v[3];
    KG.vpmaxx = v[2] + v[0];
    KG.vpmaxy = v[3] + v[1];
}

/* the draw screen's byte address of (x, y) without the segment (calcadr); aodd the odd line */
static unsigned int calcadr(unsigned int x, unsigned int y) {
    unsigned int a;
    aodd = 0;
    if (KG.drawscrheight) { aodd = (unsigned char)(y & 1); y >>= 1; }
    a = (unsigned int)((unsigned char)y >> 3) * 0x500;
    a >>= KG.drawscrwidth ? 1 : 2;
    a |= y & 7;
    return a + (x & 0xFFF8);
}

/* plane p's byte at adr + 8 * k (scanline.setup) */
static unsigned char __far *rowp(unsigned char p, unsigned int adr) {
    unsigned char seg = KG.dsbp0adr[p];
    seg = aodd ? (unsigned char)(seg & 0xE0) : (unsigned char)(seg << 4);
    return (unsigned char __far *)(((unsigned long)(KG.banka | (p & 1)) << 16) + ((unsigned int)seg << 8) + adr);
}

/* the edges' bits from the screen, the scanline written (paste.scanline) */
static unsigned char merge(unsigned char brush, unsigned char mask, unsigned char scr) {
    return (unsigned char)((brush & (unsigned char)~mask) | (scr & mask));
}
static void paste_line(unsigned char p, unsigned int adr) {
    unsigned char __far *q = rowp(p, adr);
    unsigned char k;
    if (abw != 1) {
        SCANL[0] = merge(SCANL[0], maskl[ab2sl], q[0]);
        SCANL[abw - 1] = merge(SCANL[abw - 1], maskr[abor], q[(unsigned int)(abw - 1) * 8]);
    } else SCANL[0] = merge(SCANL[0], (unsigned char)(maskl[ab2sl] | maskr[abor]), q[0]);
    for (k = 0; k < abw; k++) q[(unsigned int)k * 8] = SCANL[k];
}
static void clr_line(unsigned char p, unsigned char bit, unsigned int adr) {
    unsigned char k, v = (KG.drawpens[0] & bit) ? 0xFF : 0;
    for (k = 0; k < abw; k++) SCANL[k] = v;
    paste_line(p, adr);
}

/* x's bits within its byte, the byte columns (s.bltread / paste) */
static void geom(void) {
    unsigned int n;
    ab2sl = (unsigned char)(ax & 7);
    ax &= 0xFFF8;
    n = adx + ab2sl;
    abor = (unsigned char)(n & 7);
    abw = (unsigned char)(n >> 3);
    if (abor) abw++;
}

/* VIEWPORT CLR (0), GCOPY (1), CUT (2): the area to the brush, cleared with pen 0 */
void _bas_gcut(void) {
    unsigned char cmd = _bas_g[19], plane, bit;
    unsigned int *v = (unsigned int *)&_bas_g[1], t = 0, y0, sx;
    if (!cmd) { ax = (unsigned int)KG.vpminx; ay = (unsigned int)KG.vpminy; adx = KG.vpwx; ady = KG.vphy; }
    else { ax = v[0]; ay = v[1]; adx = v[2]; ady = v[3]; }
    sx = ax;
    geom();
    if (cmd) {
        BRUSH[0] = (unsigned char)sx;
        BRUSH[1] = (unsigned char)(ax >> 8);
        *(unsigned int *)&BRUSH[2] = ay;
        *(unsigned int *)&BRUSH[4] = adx;
        *(unsigned int *)&BRUSH[6] = ady;
        BRUSH[8] = KG.drawscrdepth;
        BRUSH[9] = abw;
        apos = 10;
    }
    y0 = ay;
    do {
        unsigned int adr = calcadr(ax, y0);
        for (plane = 0, bit = 1; bit; plane++, bit <<= 1) {
            if (!(KG.drawscrbp & bit)) continue;
            if (cmd) {
                unsigned char __far *q = rowp(plane, adr);
                unsigned char k, s2;
                for (k = 0; k < abw; k++) SCANL[k] = q[(unsigned int)k * 8];
                SCANL[abw - 1] &= (unsigned char)~maskr[abor];
                for (s2 = ab2sl; s2; s2--) {         /* (shiftleft) */
                    unsigned char c = 0;
                    signed int i;
                    for (i = abw - 1; i >= 0; i--) {
                        unsigned char b = SCANL[i];
                        SCANL[i] = (unsigned char)((b << 1) | c);
                        c = b >> 7;
                    }
                }
                for (k = 0; k < abw; k++) BRUSH[apos++] = SCANL[k];
                if (cmd != 2) continue;
            }
            clr_line(plane, bit, adr);
        }
        t++;
        y0++;
    } while (t != ady);
}

/* GCOPY / CUT's size: width * height in the planes of the last SCREEN DEF
 * under the brush's 4 pages (check_1k) */
void _bas_gcheck1k(void) {
    unsigned int *v = (unsigned int *)&_bas_g[1];
    unsigned long p = (unsigned long)v[2] * v[3];
    unsigned char a, d = PLENUM;
    if ((p >> 16) & 0xFF) raise(16);
    a = (unsigned char)(p >> 8);
    if (d < 4) { a >>= 1; if (d < 2) { a >>= 1; if (d < 1) a >>= 1; } }
    if (a >= 4) raise(14);
}

/* --- LOADIFF (kg65.loadiff: s.paste with s.OpenIFFr, s.GetRaster) --- */

#define BYTEBUF ((unsigned char *)0x1D00)    /* the kernel's IFF byte buffer */
extern void _bas_iffclose(void);
/* FORM ILBM BMHD CMAP CAMG BODY (ASCII) */
static const unsigned char iffid[] = {
    0x46, 0x4F, 0x52, 0x4D,
    0x49, 0x4C, 0x42, 0x4D,
    0x42, 0x4D, 0x48, 0x44,
    0x43, 0x4D, 0x41, 0x50,
    0x43, 0x41, 0x4D, 0x47,
    0x42, 0x4F, 0x44, 0x59};
static unsigned char lbflag, bufidx, bufsize, slidx, iffdisk;
static unsigned char chunklen[4];

/* an IFF error: the file closed first (exit_GKI_disk_op; the error number is
 * what A held at the kernel's abort: the I/O status, or a chunk id's byte) */
static void ifferr(unsigned char e) {
    _bas_iffclose();
    raise(e);
}

/* read_byte: the next byte (at the end of the file: x, the caller's X) */
static unsigned char rdbyte(unsigned char x) {
    unsigned char c;
    if (cbm_readst() & 0x40) return x;
    c = cbm_k_basin();
    if (cbm_readst() & 0x87) ifferr(cbm_readst() & 0x87);
    return c;
}

/* getXbytes: n bytes into the byte buffer */
static void getx(unsigned char n) {
    unsigned char k;
    for (k = 0; k != n; k++) BYTEBUF[k] = rdbyte(k);
}

/* findchunk: the byte buffer's first 4 bytes are chunk id i (compared from
 * the last: idbyte the first that is not) */
static unsigned char idbyte;
static unsigned char isid(unsigned char i) {
    unsigned char k = 4;
    do {
        k--;
        if (BYTEBUF[k] != iffid[i + k]) { idbyte = BYTEBUF[k]; return 0; }
    } while (k);
    return 1;
}
static unsigned char readid(unsigned char i) {
    getx(4);
    return isid(i);
}
static void needid(unsigned char i) {
    if (!readid(i)) ifferr(idbyte);
}

static void loadbuf(void) {
    bufidx = 0;
    bufsize = 0;
    lbflag = 1;
    if (!chunklen[2]) {
        bufsize = chunklen[3];
        getx(chunklen[3]);
        return;
    }
    {
        unsigned char k = 0;
        do { BYTEBUF[k] = rdbyte(k); } while (++k);
    }
    chunklen[2]--;
}

static unsigned char nextbyte(void) {
    unsigned char c = BYTEBUF[bufidx];
    if (++bufidx == bufsize) loadbuf();
    return c;
}

/* s.GetRaster: a scanline's bytes (ByteRun1) */
static void getraster(void) {
    slidx = 0;
    if (!lbflag) loadbuf();
    for (;;) {
        unsigned char c = nextbyte(), n;
        if (c == 128) continue;                 /* (a no-op, the width not tested) */
        if (c > 128) {
            unsigned char v;
            n = (unsigned char)(0 - c);
            v = nextbyte();
            do SCANL[slidx++] = v; while (n--);
        } else {
            n = c;
            do SCANL[slidx++] = nextbyte(); while (n--);
        }
        if (slidx >= abrushw) break;
    }
}

/* PALETTE screen, colour, r, g, b without its errors (kg65.setpalette) */
static void setpal(unsigned char y, unsigned char c, unsigned char *rgb) {
    if (palette(y, c, rgb, 1)) return;
    if ((unsigned char)(y << 2) == KG.viewscreen) {
        unsigned char i = color2index(KG.viewscreen, c);
        RED[i] = rgb[0];
        GREEN[i] = rgb[1];
        BLUE[i] = rgb[2];
    }
}

/* s.OpenIFFr: FORM, ILBM, BMHD (the brush's size), then CAMG / CMAP / other
 * chunks to the BODY */
static void iffheader(void) {
    lbflag = 0;
    needid(0);
    getx(4);
    needid(4);
    needid(8);
    getx(4);
    getx(BYTEBUF[3]);
    adx = (unsigned int)BYTEBUF[0] << 8 | BYTEBUF[1];
    abrushw = (unsigned char)(adx >> 3);
    ady = (unsigned int)BYTEBUF[2] << 8 | BYTEBUF[3];
    adepth = (unsigned char)(BYTEBUF[8] - 1);
    for (;;) {
        if (readid(16)) {                       /* CAMG: read, not used */
            getx(4);
            getx(BYTEBUF[3]);
            continue;
        }
        if (isid(12)) {                         /* CMAP: 4-bit colours */
            unsigned int len;
            unsigned char col = 0;
            getx(4);
            len = (unsigned int)BYTEBUF[2] << 8 | BYTEBUF[3];
            if (len & 1) len++;
            for (;;) {
                unsigned char rgb[3], k;
                if (!(len >> 8) && (unsigned char)len < 3) break;
                getx(3);
                for (k = 0; k < 3; k++) rgb[k] = BYTEBUF[k] >> 4;
                setpal(KG.drawscreen >> 2, col, rgb);
                len -= 3;
                col++;
            }
            getx((unsigned char)len);
            continue;
        }
        if (isid(20)) {                         /* BODY: its length, then the rows */
            unsigned char k;
            getx(4);
            for (k = 0; k < 4; k++) chunklen[k] = BYTEBUF[k];
            return;
        }
        {                                       /* another chunk: skipped */
            unsigned int n;
            getx(4);
            for (n = (unsigned int)BYTEBUF[2] << 8 | BYTEBUF[3]; n; n--) rdbyte((unsigned char)(n - 1));
            if (cbm_readst() & 0x87) ifferr(cbm_readst() & 0x87);
        }
    }
}

/* s.paste: the brush (or the file's picture) at (ax, ay), cut to the screen */
static void paste_(void) {
    unsigned int t = 0, y0, w, h;
    unsigned char plane, bit;
    w = (KG.drawscrwidth ? 640 : 320) - ax;
    if (w < adx) adx = w;
    h = (KG.drawscrheight ? 400 : 200) - ay;
    if (h < ady) ady = h;
    geom();
    y0 = ay;
    do {
        unsigned int adr = calcadr(ax, y0);
        for (plane = 0, bit = 1; bit; plane++, bit <<= 1) {
            unsigned char k, s2;
            if (!(KG.drawscrbp & bit)) continue;
            if (adepth > KG.drawscrdepth) continue;
            if (KG.drawscrdepth > adepth) { clr_line(plane, bit, adr); continue; }
            if (iffdisk) getraster();
            else for (k = 0; k < abrushw; k++) SCANL[k] = BRUSH[apos++];
            for (s2 = ab2sl; s2; s2--) {             /* (shiftright: bwidth + 1 bytes) */
                unsigned char c = 0, i;
                for (i = 0; i <= abw; i++) {
                    unsigned char b = SCANL[i];
                    SCANL[i] = (unsigned char)((b >> 1) | (c << 7));
                    c = b & 1;
                }
            }
            paste_line(plane, adr);
        }
        t++;
        y0++;
    } while (t != ady);
}

/* PASTE x, y: the brush at (x, y), cut to the screen */
void _bas_gpaste(void) {
    unsigned int *v = (unsigned int *)&_bas_g[1];
    ax = v[0];
    ay = v[1];
    adx = *(unsigned int *)&BRUSH[4];
    ady = *(unsigned int *)&BRUSH[6];
    adepth = BRUSH[8];
    abrushw = BRUSH[9];
    apos = 10;
    iffdisk = 0;
    paste_();
}

/* LOADIFF: the picture of the open input channel at (0, 0) */
void _bas_gloadiff(void) {
    ax = ay = 0;
    iffdisk = 1;
    iffheader();
    paste_();
}

/* --- SAVEIFF (kg65.saveiff: WriteBMHD, WriteCMAP, WriteBODY; two passes) --- */

static unsigned long chunkn, formlen, bmhdlen, cmaplen, bodylen;
static unsigned char wpass;

/* WriteByte: counted (pass 1) or written (pass 2) */
static void wbyte(unsigned char c) {
    if (!(wpass & 1)) { chunkn++; return; }
    cbm_k_bsout(c);
    if (cbm_readst() & 0x87) ifferr(cbm_readst() & 0x87);
}
static void wid(unsigned char i) {
    unsigned char k;
    for (k = 0; k < 4; k++) wbyte(iffid[i + k]);
}
static void wlong(unsigned long v) {
    wbyte((unsigned char)(v >> 24));
    wbyte((unsigned char)(v >> 16));
    wbyte((unsigned char)(v >> 8));
    wbyte((unsigned char)v);
}
static void wword(unsigned int v) {
    wbyte((unsigned char)(v >> 8));
    wbyte((unsigned char)v);
}
/* ClearChunkLen: the count added to the FORM's (pass 1 only) */
static void clrchunk(void) {
    if (wpass & 1) return;
    formlen += chunkn;
    chunkn = 0;
}

void _bas_gsaveiff(void) {
    unsigned int wd, ht;
    chunkn = 0;
    formlen = 0xFFFFFFF8UL;
    wpass = 0xF8;
    do {
        unsigned char k, n, y;
        wd = KG.drawscrwidth ? 640 : 320;
        ht = KG.drawscrheight ? 400 : 200;
        /* BMHD */
        wid(0);
        wlong(formlen);
        wid(4);
        wid(8);
        wlong(bmhdlen);
        clrchunk();
        wword(wd);
        wword(ht);
        for (k = 0; k < 4; k++) wbyte(0);
        wbyte((unsigned char)(KG.drawscrdepth + 1));
        wbyte(0);
        wbyte(1);
        for (k = 0; k < 3; k++) wbyte(0);
        wbyte(1);
        wbyte(1);
        wword(wd);
        wword(ht);
        bmhdlen = chunkn;
        /* CMAP: the draw screen's colours, the nibbles doubled (8 planes: one colour) */
        wid(12);
        wlong(cmaplen);
        clrchunk();
        n = KG.drawscrdepth < 7 ? (unsigned char)(2 << KG.drawscrdepth) : 0;
        y = 0;
        do {
            unsigned char rgb[3];
            palette(KG.drawscreen >> 2, y, rgb, 0);
            for (k = 0; k < 3; k++) wbyte((unsigned char)(rgb[k] << 4) | rgb[k]);
            y++;
        } while (y < n);
        if (chunkn & 1) wbyte(0);
        cmaplen = chunkn;
        /* BODY: each row's planes, ByteRun1 where it is shorter */
        wid(20);
        wlong(bodylen);
        clrchunk();
        {
            unsigned char bw = KG.drawscrcols, p, bit;
            unsigned int line;
            abw = bw;
            for (line = 0; line < ht; line++) {
                unsigned int adr = calcadr(0, line);
                for (p = 0, bit = 1; bit; p++, bit <<= 1) {
                    unsigned char __far *q;
                    unsigned char yy, x, z, c;
                    if (!(KG.drawscrbp & bit)) continue;
                    q = rowp(p, adr);
                    for (k = 0; k < bw; k++) SCANL[k] = q[(unsigned int)k * 8];
                    /* CompressScanLine */
                    yy = 0;
                    x = 0;
                    for (;;) {
                        z = 0xFF;
                        c = SCANL[yy];
                        for (;;) {
                            z++;
                            yy++;
                            if (yy > bw) goto done;
                            if (yy == bw || c != SCANL[yy]) break;
                        }
                        if (z >= 2) {
                            BYTEBUF[x++] = (unsigned char)(0 - z);
                            BYTEBUF[x++] = SCANL[yy - 1];
                        } else {
                            BYTEBUF[x++] = z;
                            c = SCANL[yy - 1];
                            do BYTEBUF[x++] = c; while ((signed char)--z >= 0);
                        }
                    }
                done:
                    if (x < bw) {
                        k = x;
                        x = 0;
                        do wbyte(BYTEBUF[x++]); while (--k);
                    } else {
                        wbyte((unsigned char)(bw - 1));
                        for (k = 0; k < bw; k++) wbyte(SCANL[k]);
                    }
                }
            }
        }
        bodylen = chunkn;
        if (chunkn & 1) wbyte(0);
        clrchunk();
        wpass++;
    } while (wpass & 1);
}

/* --- CHAR (kg65.char, petpok / petctr / curcntl, tiltchr, mirrorchr, flipchr) --- */

/* _bas_g: column, row (2), height, width, direction; the string at _bas_gstr;
 * the font's 28-bit address at _bas_gfont */
unsigned char *_bas_gstr;
unsigned long _bas_gfont;
#define WBUFR ((unsigned char *)0x1D00)
static const unsigned char bitsup[8] = {1, 2, 4, 8, 16, 32, 64, 128};
static unsigned char cbuf1[8], cbuf2[8], ccount, fsta, frvs, funder, cmod, wcol, chrcol, clipcol;
static unsigned int winy, chrrow, cliprow;

/* CHAR's font address (__float_a, BASIC's getwrd: 0 .. 2^32 - 1): its low 16
 * bits in bank 2 unless the third byte gives a bank (and the fourth a megabyte) */
extern unsigned long _bas_fa;
extern void _bas_getadr32(void);
void _bas_gfontset(void) {
    _bas_getadr32();
    if ((_bas_fa >> 16) & 0xFF) _bas_gfont = _bas_fa;
    else _bas_gfont = 0x20000UL | (unsigned int)_bas_fa;
}

static unsigned char cgetc(unsigned char *c) {
    if (ccount >= _bas_gstr[0]) return 1;
    *c = _bas_gstr[1 + ccount];
    return 0;
}

static void ccopy(void) { chrcol = _bas_g[0]; chrrow = *(unsigned int *)&_bas_g[1]; }

/* a character: its screen code (0: printable), or a control's effect (petpok) */
static unsigned char petpok(unsigned char *a) {
    unsigned char c = *a, x;
    if (c >= 0x20 && c < 0x40) return 0;
    if (c >= 0x40 && c < 0x60) { *a = c - 0x40; return 0; }
    if (c >= 0x60 && c < 0x80) { *a = c - 0x20; return 0; }
    if (c >= 0xA0 && c < 0xC0) { *a = c - 0x40; return 0; }
    if (c >= 0xC0) { *a = (c == 0xFF ? 0x5E : c) & 0x7F; return 0; }
    x = c;
    switch (x) {
    case 2: fsta |= 1; return x;
    case 6: cmod ^= 0x08; return x;
    case 9: fsta |= 4; return x;
    case 26: cmod ^= 0x04; return x;
    case 15: fsta |= 2; return x;
    case 0x12: frvs = 0xFF; return x;
    case 0x92: frvs = 0; return x;
    case 25: {
        unsigned char n;
        if (cgetc(&n)) return x;
        cmod &= 0x3F;
        if (n == 0x2D) cmod |= 0x80;
        else if (n == 0x2B) cmod |= 0x40;
        else return x;
        ccount++;
        return x;
    }
    case 21: funder ^= 0xFF; return x;
    case 24: fsta |= 8; return x;
    }
    /* cursor controls (curcntl): the base position moved */
    {
        unsigned int *row = (unsigned int *)&_bas_g[1];
        if (x == 17) *row += 8;
        else if (x == 145) *row -= 8;
        else if (x == 157) _bas_g[0]--;
        else if (x == 29) _bas_g[0]++;
        if (x == 147 || x == 19) { _bas_g[0] = 0; *row = 0; }
        ccopy();
    }
    return 0xFF;
}

static void tiltchr(void) {
    signed char x, y;
    for (x = 0, y = 7; y >= 0; x++, y--) {
        unsigned char mask = (cmod & 0x80) ? bitsup[y] : bitsdn[y], b = cbuf1[x];
        signed char k;
        for (k = 7; k >= 0; k--) {
            unsigned char c;
            if (cmod & 0x80) { c = b >> 7; b <<= 1; }
            else { c = b & 1; b >>= 1; }
            if (c) cbuf2[k] |= mask;
            else cbuf2[k] &= (unsigned char)~mask;
        }
    }
    for (x = 0; x < 8; x++) cbuf1[x] = cbuf2[x];
}

/* the character's 8 rows (cbuf1), height times each, at column clipcol (one byte) */
static void cdraw(void) {
    unsigned char y, h, plane, bit;
    for (y = 0; y < 8; y++) {
        for (h = _bas_g[3]; h; h--) {
            if (clipcol < wcol && cliprow < winy) {
                unsigned int adr = calcadr((unsigned int)clipcol * 8, cliprow);
                unsigned char def = cbuf1[y];
                for (plane = 0, bit = 1; bit; plane++, bit <<= 1) {
                    unsigned char __far *q;
                    unsigned char b = 0;
                    if (!(KG.drawscrbp & bit)) continue;
                    q = rowp(plane, adr);
                    if (!(fsta & 1)) b = *q;
                    if (fsta & 0x0E) {
                        if (KG.drawpens[0] & bit) {
                            unsigned char a = def;
                            if (fsta & 4) a &= b;
                            if (fsta & 2) a |= b;
                            if (fsta & 8) a ^= b;
                            b = a;
                        }
                    } else if (KG.drawpens[0] & bit) b = (unsigned char)((b & (unsigned char)~def) | def);
                    else b &= (unsigned char)~def;
                    *q = b;
                }
            }
            cliprow++;
        }
    }
}

void _bas_gchar(void) {
    unsigned char *row_lo = &_bas_g[1];
    ccount = fsta = frvs = funder = cmod = 0;
    wcol = (unsigned char)(KG.vpmaxx >> 3);
    winy = (unsigned int)KG.vpminy + KG.vphy;
    for (;;) {
        unsigned char c, x, k, hw, w = _bas_g[4];
        unsigned int *row = (unsigned int *)row_lo;
        unsigned long cp;
        ccopy();
        for (;;) {
            if (cgetc(&c)) return;
            ccount++;
            if (!petpok(&c)) break;
        }
        cp = (_bas_gfont & 0xFFFF0000UL) | (unsigned int)((unsigned int)_bas_gfont + (unsigned int)c * 8);
        for (k = 0; k < 8; k++) cbuf1[k] = ((unsigned char __far *)cp)[k];
        clipcol = chrcol;
        cliprow = chrrow;
        cbuf1[7] |= funder;
        for (k = 0; k < 8; k++) { cbuf1[k] ^= frvs; cbuf2[k] = 0; }
        if (cmod & 0xC0) tiltchr();
        if (cmod & 0x04)
            for (k = 0; k < 8; k++) {
                unsigned char b = cbuf1[k], r = 0, n;
                for (n = 0; n < 8; n++) { r = (unsigned char)((r << 1) | (b & 1)); b >>= 1; }
                cbuf1[k] = r;
            }
        if (cmod & 0x08)
            for (k = 0; k < 4; k++) { unsigned char t = cbuf1[k]; cbuf1[k] = cbuf1[7 - k]; cbuf1[7 - k] = t; }
        hw = (unsigned char)(w - 1);
        if (hw) {
            /* the widths: each row's bits w times each, chained through up to 16 bytes */
            unsigned char len = w < 4 ? 3 : w < 7 ? 6 : w < 11 ? 10 : 16;
            for (k = 0; k < 128; k++) WBUFR[k] = 0;
            for (x = 0; x < 8; x++) {
                unsigned char yb;
                for (yb = 0; yb < 8; yb++) {
                    unsigned char hold = (cbuf1[x] & bitsdn[yb]) ? 1 : 0, n = w;
                    do {
                        unsigned char cc = hold, j;
                        for (j = 0; j < len; j++) {
                            unsigned char b = WBUFR[j * 8 + x];
                            WBUFR[j * 8 + x] = (unsigned char)((b << 1) | cc);
                            cc = b >> 7;
                        }
                    } while (--n);
                }
            }
        }
        for (;;) {
            if (w != 1)
                for (k = 0; k < 8; k++) cbuf1[k] = WBUFR[(unsigned char)(hw * 8) + k];
            cdraw();
            chrcol++;
            clipcol++;
            cliprow = chrrow;
            hw--;
            if (hw & 0x80) break;
        }
        /* the direction: 1 up, 2 right, 4 down, 8 left (with the kernel's wraps) */
        {
            unsigned char d = _bas_g[5];
            unsigned int h8 = (unsigned int)_bas_g[3] * 8;
            if (d & 1) {
                *row -= h8;
                if (*row & 0x8000) {
                    *row = winy - h8;
                    _bas_g[0] -= w;
                    if (_bas_g[0] & 0x80) _bas_g[0] = (unsigned char)(wcol - (unsigned char)(h8 >> 8));
                }
            }
            if (d & 2) {
                _bas_g[0] += w;
                if (_bas_g[0] >= wcol) {
                    _bas_g[0] = 0;
                    *row += h8;
                    if (*row >= winy) *row = 0;
                }
            }
            if (d & 4) {
                *row += h8;
                if (*row >= winy) {
                    *row = 0;
                    _bas_g[0] += w;
                    if (_bas_g[0] >= wcol) _bas_g[0] = 0;
                }
            }
            if (d & 8) {
                _bas_g[0] -= w;
                if (_bas_g[0] & 0x80) {
                    _bas_g[0] = (unsigned char)(wcol - (unsigned char)(h8 >> 8));
                    *row -= h8;
                    if (*row & 0x8000) *row = winy - h8;
                }
            }
        }
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
