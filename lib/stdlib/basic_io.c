/* basic_io.c — OPEN, CLOSE and ST of BASIC 65 programs compiled by basic45 --compile (#369)
 *
 * As BASIC 65's (b65.src: open, close, paoc):
 *   _bas_open: OPEN __bas_olf [, __bas_odev [, __bas_osa [, name]]], the
 *     values given in __bas_ocnt (1-4), the name a string at __bas_sa (none:
 *     0). Without a device the default drive ($1106), secondary address 0;
 *     a device without one: 0 below device 4, else $FF (none). The name read
 *     from bank 0. The KERNAL's error raises BASIC's of that number.
 *   _bas_close: CLOSE __bas_n.
 *   _bas_st: ST (the KERNAL's I/O status, READST) into __bas_n.
 * As BASIC 65's DOS statements (b65.src: dopen, append, dclose, dosprs,
 * sendp's tabld, Read_DS, Get_DS):
 *   _bas_dopen / _bas_append: DOPEN# / APPEND# __bas_dlf, the name at
 *     __bas_sa ("@" first: replace; not for APPEND), drive __bas_ddrv, unit
 *     __bas_dunit and record length __bas_drcl as __bas_dflg gives them (bit
 *     0 W, 1 L, 2 U, 3 D). The command string as the ROM's ("@0:name",
 *     ",W,S" / ",L," and the length; "0:name,A"), a free secondary address
 *     $62-$6E (?TOO MANY FILES), the file opened. DOPEN forgets DS$.
 *   _bas_dclose: DCLOSE# __bas_dlf (bit 4 of __bas_dflg: given), else all
 *     files of the unit (CLOSE_ALL).
 *   _bas_ds / _bas_dss: DS into __bas_n, DS$ into __bas_sr's buffer: the
 *     drive's error channel read (lf 0, sa 15) when DS$ is not kept, from
 *     the KERNAL's device ($BA) if a drive, else the last DOS statement's
 *     unit. OPEN of a drive, DOPEN forget it.
 *   _bas_chainprep: DLOAD (__bas_dflg bit 5 clear: the name at __bas_sa,
 *     D / U as DOPEN's; "$..." as it is, else "0:name") or LOAD (bit 5:
 *     the name as it is, the device __bas_dunit (bit 6, else the default
 *     drive; 1 or 4-30), the secondary address __bas_osa (bit 7)) checked
 *     and set up for basic_rt.s45's __bas_chain: the name at $0280 (BASIC's
 *     input buffer; up to 31 characters), __bas_clen, __bas_cdev, __bas_csa.
 *   _bas_xbegin / _bas_xput / _bas_xopen / _bas_xget / _bas_xdone: the
 *     chain block (REM PRAGMA EXPORT / IMPORT): at $87F0000 (attic RAM),
 *     "B45V", the exporter's id (__bas_n, __bas_m), then each variable in
 *     the map's order: a type (0 float: 5 bytes, 1 int: 2, 2 string: a
 *     length and the characters). _bas_xopen: __bas_n := 1 when the block
 *     holds the id's; _bas_xget: variable __bas_n of type __bas_m into
 *     __float_a / __bas_n / __bas_sr's buffer; _bas_xdone forgets the block.
 * The channels (PRINT#, CMD, INPUT#, GET#) are basic_rt.s45's __bas_chkout,
 * __bas_chkin and __bas_clrch.
 */
#include <cbm.h>

extern unsigned char *_bas_sa, *_bas_sr;
extern int _bas_n, _bas_m;
extern unsigned char _float_a[5];
extern void _bas_error(void);

unsigned char _bas_olf, _bas_odev, _bas_osa, _bas_ocnt;
unsigned char _bas_dlf, _bas_ddrv, _bas_dunit, _bas_drcl, _bas_dflg;
unsigned char _bas_clen, _bas_cdev, _bas_csa;

#define DEFDRV (*(volatile unsigned char *)0x1106)      /* the default drive */
#define KFA (*(volatile unsigned char __far *)0xBAUL)    /* the KERNAL's device (fa) */

static unsigned char ds[40];     /* DS$: a length, up to 39 characters (0: none read) */
static unsigned char dosfa;      /* the last DOS statement's unit (0: none yet) */
static unsigned char cmd[64];

static void kerr(unsigned char e) {
    _bas_n = e ? e : 30;        /* (0: BREAK, as BASIC's erexit) */
    _bas_error();
}

/* the DOS statement's unit (dosprs: the default drive, U given: checked as
 * the ROM's unit) */
static unsigned char unit(void) {
    unsigned char u = DEFDRV;
    if (_bas_dflg & 4) {
        u = _bas_dunit;
        if (u > 30) kerr(9);
        if (u == 1) u = DEFDRV;
        else if (u < 4) kerr(9);
    }
    return dosfa = u;
}

static void dosopen(unsigned char append) {
    unsigned char *nm = _bas_sa, len, n = 0, k, sa, e, u;
    if (!_bas_dlf) kerr(14);
    if ((_bas_dflg & 8) && _bas_ddrv > 9) kerr(14);
    if ((_bas_dflg & 2) && (_bas_drcl == 0 || _bas_drcl == 255)) kerr(14);
    u = unit();
    len = nm[0];
    nm++;
    if (!len) kerr(8);
    if (nm[0] == '@') {          /* (replace) */
        if (append) kerr(11);
        cmd[n++] = '@';
        nm++;
        if (!--len) kerr(8);
    }
    cmd[n++] = '0' + (_bas_dflg & 8 ? _bas_ddrv : 0);
    cmd[n++] = ':';
    for (k = 0; k < len && n < sizeof cmd - 4; k++) cmd[n++] = nm[k];
    if (append) {
        cmd[n++] = ',';
        cmd[n++] = 0x41;      /* A (PETSCII: cc45's 'A' is the shifted letter) */
    } else if (_bas_dflg & 3) {
        cmd[n++] = ',';
        if (_bas_dflg & 2) {
            cmd[n++] = 0x4C;      /* L */
            cmd[n++] = ',';
            cmd[n++] = _bas_drcl == 1 ? 0 : _bas_drcl;
        } else {
            cmd[n++] = 0x57;      /* W */
            cmd[n++] = ',';
            cmd[n++] = 0x53;      /* S */
        }
    }
    /* a free secondary address (the KERNAL's LKUPSA: carry clear when used) */
    for (sa = 0x62; ; sa++) {
        if (sa == 0x6F) kerr(1);
        if (__jsr(0xFF62, .Y = sa).Carry) break;
    }
    cbm_setnam((char *)cmd, n);
    cbm_setlfs(_bas_dlf, u, sa);
    cbm_clrchn();
    cbm_setbnk(0, 0);
    if (__jsr(0xFFC0, .A -> e).Carry) kerr(e);
}

void _bas_dopen(void) {
    ds[0] = 0;
    dosopen(0);
}

void _bas_append(void) {
    dosopen(1);
}

void _bas_dclose(void) {
    unsigned char u = unit(), e;
    if (_bas_dflg & 16) {
        if (__jsr(0xFFC3, .A = _bas_dlf, .Carry = 0, .A -> e).Carry) kerr(e);
    } else __jsr(0xFF50, .A = u);
}

/* DS$ read from the drive's error channel (Read_DS) */
static void dsread(void) {
    unsigned char u = KFA, c, n = 0, e;
    if (u < 8) u = dosfa ? dosfa : DEFDRV;
    cbm_setlfs(0, u, 0x6F);
    cbm_setnam(0, 0);
    cbm_open();
    if (__jsr(0xFFC6, .X = 0, .A -> e).Carry) {
        cbm_clrchn();
        __jsr(0xFFC3, .A = 0, .Carry = 1);
        ds[0] = 0;
        kerr(e);
    }
    for (;;) {
        c = (unsigned char)cbm_chrin();
        if (c < ' ') break;
        ds[++n] = c;
        if (n == sizeof ds - 1) break;
    }
    ds[0] = n;
    cbm_clrchn();
    __jsr(0xFFC3, .A = 0, .Carry = 1);     /* (not a real close) */
}

void _bas_ds(void) {
    if (!ds[0]) dsread();
    _bas_n = (ds[1] & 15) * 10 + (ds[2] & 15);
}

void _bas_dss(void) {
    unsigned char k;
    if (!ds[0]) dsread();
    for (k = 0; k <= ds[0]; k++) _bas_sr[k] = ds[k];
}

void _bas_open(void) {
    unsigned char dev, sa, e, len = 0;
    unsigned char *nm = _bas_sa;
    if (_bas_ocnt < 2) {
        dev = DEFDRV;
        sa = 0;
    } else {
        dev = _bas_odev;
        if (_bas_ocnt < 3) sa = dev < 4 ? 0 : 0xFF;
        else sa = _bas_osa;
    }
    if (nm) { len = nm[0]; nm++; }
    if ((dev == 1 ? DEFDRV : dev) >= 8) ds[0] = 0;     /* (a drive: DS$ forgotten) */
    cbm_setbnk(0, 0);
    cbm_setlfs(_bas_olf, dev, sa);
    cbm_setnam((char *)nm, len);
    if (__jsr(0xFFC0, .A -> e).Carry) kerr(e);
}

void _bas_close(void) {
    unsigned char e;
    if (__jsr(0xFFC3, .A = (unsigned char)_bas_n, .Carry = 0, .A -> e).Carry) kerr(e);
}

void _bas_st(void) {
    _bas_n = (unsigned char)cbm_readst();
}

void _bas_chainprep(void) {
    unsigned char *nm = _bas_sa, len = 0, n = 0, k;
    unsigned char *out = (unsigned char *)0x0280;
    if (nm) { len = nm[0]; nm++; }
    if (_bas_dflg & 32) {
        /* LOAD (plsv) */
        unsigned char d = _bas_dflg & 64 ? _bas_dunit : DEFDRV;
        if (d != 1 && d < 4) kerr(9);
        _bas_cdev = d;
        _bas_csa = _bas_dflg & 128 ? _bas_osa : 0;
        for (k = 0; k < len && n < 31; k++) out[n++] = nm[k];
    } else {
        /* DLOAD (dosprs, dload) */
        if ((_bas_dflg & 8) && _bas_ddrv > 9) kerr(14);
        _bas_cdev = unit();
        _bas_csa = 0;
        if (!len) kerr(8);
        if (nm[0] == '@') kerr(11);
        if (nm[0] != '$') {
            out[n++] = '0' + (_bas_dflg & 8 ? _bas_ddrv : 0);
            out[n++] = ':';
        }
        for (k = 0; k < len && n < 31; k++) out[n++] = nm[k];
    }
    _bas_clen = n;
}

#define XBLOCK 0x87F0000UL
static unsigned char __far *xp;

void _bas_xbegin(void) {
    xp = (unsigned char __far *)XBLOCK;
    xp[0] = 0x42; xp[1] = 0x34; xp[2] = 0x35; xp[3] = 0x56;     /* "B45V" (PETSCII) */
    xp[4] = (unsigned char)_bas_n; xp[5] = (unsigned char)(_bas_n >> 8);
    xp[6] = (unsigned char)_bas_m; xp[7] = (unsigned char)(_bas_m >> 8);
    xp += 8;
}

void _bas_xput(void) {
    unsigned char k;
    *xp++ = (unsigned char)_bas_m;
    if (_bas_m == 0) {
        for (k = 0; k < 5; k++) *xp++ = _float_a[k];
    } else if (_bas_m == 1) {
        *xp++ = (unsigned char)_bas_n;
        *xp++ = (unsigned char)(_bas_n >> 8);
    } else {
        unsigned char *t = _bas_sa;
        for (k = 0; k <= t[0]; k++) *xp++ = t[k];
    }
}

void _bas_xopen(void) {
    unsigned char __far *b = (unsigned char __far *)XBLOCK;
    _bas_n = b[0] == 0x42 && b[1] == 0x34 && b[2] == 0x35 && b[3] == 0x56 &&
             b[4] == (unsigned char)_bas_n && b[5] == (unsigned char)(_bas_n >> 8) &&
             b[6] == (unsigned char)_bas_m && b[7] == (unsigned char)(_bas_m >> 8);
}

void _bas_xget(void) {
    unsigned char __far *b = (unsigned char __far *)XBLOCK + 8;
    unsigned char k, t;
    int i = _bas_n;
    /* (past the variables before it) */
    while (i--) {
        t = *b++;
        b += t == 0 ? 5 : t == 1 ? 2 : (unsigned)*b + 1;
    }
    t = *b++;
    if (t != (unsigned char)_bas_m) t = 255;    /* (another type: the variable's 0 / "") */
    if (_bas_m == 0) {
        for (k = 0; k < 5; k++) _float_a[k] = t == 0 ? b[k] : 0;
    } else if (_bas_m == 1) {
        _bas_n = t == 1 ? b[0] | (b[1] << 8) : 0;
    } else {
        unsigned char *r = _bas_sr;
        r[0] = t == 2 ? b[0] : 0;
        for (k = 1; k <= r[0]; k++) r[k] = b[k];
    }
}

void _bas_xdone(void) {
    ((unsigned char __far *)XBLOCK)[0] = 0;
}
