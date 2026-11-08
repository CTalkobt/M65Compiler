/* basic_scr.c — the text screen and colour statements of BASIC 65 programs
 * compiled by basic45 --compile (#369)
 *
 * As BASIC 65's (b65.src V920413: foreground, highlight, background,
 * border, rcolor, cursor, rcursor, window, rwindow, speed, serial_switch,
 * rspeed, vsync, c65.setpalette's physical colours): the VIC-IV's registers
 * and the editor's variables written directly, the KERNAL's PLOT ($FFF0)
 * and the editor's jump table (_set_window $E02D, _cursor $E030,
 * _palette_init $E027) called. Arguments in __bas_n / __bas_m and:
 *   _bas_fg       FOREGROUND / COLOR __bas_n: the text colour ($F1); 16 and
 *                 up the colour - 16 with the attribute bit 6
 *   _bas_hilite   HIGHLIGHT __bas_n [, __bas_m (255: none)]: errcolor ($02A7),
 *                 REM's ($02A8), keywords' ($02A9) and LIST's flags ($55)
 *   _bas_vicset   BACKGROUND / BORDER: VIC register __bas_m := __bas_n
 *   _bas_rcolor   RCOLOR(__bas_n) into __bas_n
 *   _bas_curonoff CURSOR ON (__bas_n 0) / OFF (1)
 *   _bas_curpos   CURSOR's position: column _bas_cc (__bas_m bit 0), row
 *                 _bas_cr (bit 1), style _bas_cs (bit 2): ESC F / ESC E
 *   _bas_rcursor  RCURSOR: the column into __bas_n, the row into __bas_m
 *   _bas_window   WINDOW _bas_wl, _bas_wt, _bas_wr, _bas_wb, _bas_wc (clear)
 *   _bas_rwindow  RWINDOW(__bas_n) into __bas_n: 0 the window's width, 1 its
 *                 height, 2 the screen's width, 3 its height
 *   _bas_speed    SPEED / FAST __bas_n (1: 1 MHz, 3: 3.5 MHz, else 40 MHz)
 *   _bas_fastser  FAST ON (__bas_n 1) / OFF (0): the fast serial flag
 *   _bas_rspeed   RSPEED: 1, 3 or 40 into __bas_n
 *   _bas_vsync    VSYNC __bas_n: wait for that raster line
 *   _bas_pal      PALETTE COLOR __bas_n, _bas_cr (red, 0-31), _bas_cs (green),
 *                 __bas_m (blue)
 *   _bas_palinit  PALETTE RESTORE
 */

extern int _bas_n, _bas_m;
extern void _bas_error(void);
extern void _bas_out(void);

unsigned char _bas_cc, _bas_cr, _bas_cs;
unsigned char _bas_wl, _bas_wt, _bas_wr, _bas_wb, _bas_wc;

#define VIC ((volatile unsigned char *)0xD000)
#define ZP(a) (*(volatile unsigned char __far *)a##UL)  /* the ROM's base page */
#define COLOR ZP(0xF1)
#define HELPER ZP(0x55)
#define FSERIAL ZP(0x96)
#define SCBOT ZP(0xE4)
#define SCTOP ZP(0xE5)
#define SCLF ZP(0xE6)
#define SCRT ZP(0xE7)
#define LINES ZP(0xED)
#define COLUMNS ZP(0xEE)
#define CRMODE ZP(0xD7)
#define ERRCOLOR ((volatile unsigned char *)0x02A7)
#define BLNSW (*(volatile unsigned char *)0x1120)

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

static void putc_(unsigned char c) { __jsr_raw(_bas_out, .A = c); }

void _bas_fg(void) {
    unsigned char c = (unsigned char)_bas_n;
    if (c >= 16) c = (unsigned char)((c - 16) | 0x40);
    COLOR = c;
}

void _bas_hilite(void) {
    unsigned char c = (unsigned char)_bas_n, m;
    if (_bas_m == 255) { ERRCOLOR[0] = c; return; }
    m = (unsigned char)(_bas_m & 3);
    HELPER = HELPER | (unsigned char)(m << 3);
    if (m >= 3) {
        m = 2;
        ERRCOLOR[1] = c;
    }
    ERRCOLOR[m] = c;
}

void _bas_vicset(void) {
    VIC[_bas_m] = (unsigned char)_bas_n;
}

void _bas_rcolor(void) {
    unsigned char a = (unsigned char)_bas_n;
    if (a >= 4) raise(14);
    _bas_n = a == 0 ? VIC[33] : a == 1 ? COLOR : a == 2 ? ERRCOLOR[0] : VIC[32];
}

void _bas_curonoff(void) {
    __jsr(0xE030, .Carry = _bas_n);
}

void _bas_curpos(void) {
    unsigned char row, col, blink, bad;
    __jsr(0xFFF0, .Carry = 1, .X -> row, .Y -> col);
    blink = BLNSW;                               /* the cursor off meanwhile */
    __jsr(0xE030, .Carry = 1);
    if (_bas_m & 1) col = _bas_cc;
    if (_bas_m & 2) row = _bas_cr;
    bad = __jsr(0xFFF0, .Carry = 0, .X = row, .Y = col).Carry;
    BLNSW = blink;
    if (bad) raise(14);
    if (_bas_m & 4) {
        putc_(27);
        putc_((unsigned char)(0x45 + ((_bas_cs & 1) ^ 1)));     /* ESC F flashing, ESC E solid */
    }
}

void _bas_rcursor(void) {
    unsigned char row, col;
    __jsr(0xFFF0, .Carry = 1, .X -> row, .Y -> col);
    _bas_n = col;
    _bas_m = row;
}

void _bas_window(void) {
    if (_bas_wl >= COLUMNS || _bas_wt >= LINES) raise(14);
    __jsr(0xE02D, .Carry = 0, .A = _bas_wt, .X = _bas_wl);
    if (_bas_wr > COLUMNS || _bas_wr < SCLF || _bas_wb > LINES || _bas_wb < SCTOP) raise(14);
    __jsr(0xE02D, .Carry = 1, .A = _bas_wb, .X = _bas_wr);
    if (_bas_wc == 0) putc_(0x13);
    else if (_bas_wc == 1) putc_(0x93);
    else raise(14);
}

void _bas_rwindow(void) {
    unsigned char a = (unsigned char)_bas_n;
    if (a == 0) _bas_n = SCRT - SCLF + 1;
    else if (a == 1) _bas_n = SCBOT - SCTOP + 1;
    else if (a == 2) _bas_n = (CRMODE & 0x80) ? 40 : 80;
    else if (a == 3) _bas_n = (VIC[0x31] & 0x08) ? 50 : 25;
    else raise(14);
}

void _bas_speed(void) {
    unsigned char hot, n = (unsigned char)_bas_n;
    VIC[0x2F] = 0x47;                            /* the VIC-IV's registers on */
    VIC[0x2F] = 0x53;
    hot = VIC[0x5D] & 0x80;                      /* hot registers off meanwhile */
    VIC[0x5D] &= 0x7F;
    VIC[0x54] &= 0xBF;
    VIC[0x31] &= 0xBF;
    if (n != 1) {
        if (n != 3) VIC[0x54] |= 0x40;
        VIC[0x31] |= 0x40;
    }
    VIC[0x5D] &= 0x7F;
    VIC[0x5D] |= hot;
}

void _bas_fastser(void) {
    if (_bas_n) FSERIAL = FSERIAL | 2;
    else FSERIAL = FSERIAL & 0xFD;
}

void _bas_rspeed(void) {
    _bas_n = !(VIC[0x31] & 0x40) ? 1 : !(VIC[0x54] & 0x40) ? 3 : 40;
}

void _bas_vsync(void) {
    unsigned int v = (unsigned int)_bas_n;
    unsigned char lo = (unsigned char)v;
    if (v > 511) raise(14);
    if (v & 256) {
        if (lo >= ((VIC[0x6F] & 0x80) ? (unsigned char)(262 - 256) : (unsigned char)(312 - 256))) raise(14);
        do {
            while (!(VIC[0x11] & 0x80)) {}
        } while (VIC[0x12] != lo || !(VIC[0x11] & 0x80));
        return;
    }
    /* (NTSC: the core reports no line below 6) */
    if ((VIC[0x6F] & 0x80) && lo < 6) lo = 6;
    do {
        while (VIC[0x11] & 0x80) {}
    } while (VIC[0x12] != lo || (VIC[0x11] & 0x80));
}

void _bas_pal(void) {
    unsigned char i = (unsigned char)_bas_n;
    if (_bas_cr >= 32 || _bas_cs >= 16 || (unsigned)_bas_m >= 16) raise(14);
    ((volatile unsigned char *)0xD100)[i] = _bas_cr;
    ((volatile unsigned char *)0xD200)[i] = _bas_cs;
    ((volatile unsigned char *)0xD300)[i] = (unsigned char)_bas_m;
}

void _bas_palinit(void) {
    __jsr(0xE027);
}
