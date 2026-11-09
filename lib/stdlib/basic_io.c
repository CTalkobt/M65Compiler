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
 *   _bas_dos: the DOS statement __bas_m (1 DOPEN, 2 APPEND, 3 DCLOSE, 5
 *     HEADER, 6 SCRATCH / ERASE / DELETE "name", 7 RENAME, 8 CONCAT, 9
 *     COLLECT, 10 DCLEAR, 11 BACKUP, 12 COPY on one unit) with the
 *     parameters basic45 parsed (dosprs below): the ROM's command string on
 *     the command channel (trans), or DOPEN / APPEND's file opened on a free
 *     secondary address $62-$6E (?TOO MANY FILES). DOPEN forgets DS$.
 *   _bas_record: RECORD# lf, record, byte (P on the file's unit).
 *   _bas_dos 16: DIR / CATALOG [name] [, D] [, U] [, R] [, W] [, P] (directory,
 *     ldir): "$0:name" ("_" with R) loaded on lf 0, secondary address $60, a
 *     line per entry (the blocks right-aligned in 4, the rest as the drive
 *     sends it); W: 20 characters each in columns of 21 rows below the
 *     header (the screen cleared first, the BLOCKS FREE line two rows above
 *     the bottom; dirpos), not with P or
 *     CMD; P: the screen cleared, [MORE] and a key at the bottom (STOP or Q
 *     ends it). STOP ends the listing.
 *   _bas_fread / _bas_fwrite: FREAD# / FWRITE# __bas_dlf: __bas_m bytes (0:
 *     65536) from the file (GETIN) to the 28-bit address __bas_fa, or from
 *     there to the file (BSOUT), as the ROM's fread.
 *   _bas_ds / _bas_dss: DS into __bas_n, DS$ into __bas_sr's buffer: the
 *     drive's error channel read (lf 0, sa 15) when DS$ is not kept, from
 *     the KERNAL's device ($BA) if a drive, else the last DOS statement's
 *     unit. OPEN of a drive, DOPEN forget it.
 *   _bas_chainprep: DLOAD (__bas_dflg bit 5 clear: dosprs's name, D, U;
 *     "$..." as it is, else "0:name") or LOAD (bit 5:
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
extern void _bas_out(void);
/* a character to the screen (basic_rt.s45's __bas_out: A) */
static void _bas_putc(unsigned char c) { __jsr_raw(_bas_out, .A = c); }

unsigned char _bas_olf, _bas_odev, _bas_osa, _bas_ocnt;
unsigned char _bas_dlf, _bas_dunit, _bas_drcl, _bas_dflg;
unsigned char _bas_clen, _bas_cdev, _bas_csa;

#define DEFDRV (*(volatile unsigned char *)0x1106)      /* the default drive */
#define KFA (*(volatile unsigned char __far *)0xBAUL)    /* the KERNAL's device (fa) */

static unsigned char ds[40];     /* DS$: a length, up to 39 characters (0: none read) */
static unsigned char dosfa;      /* the DOS statement's unit */
static unsigned char unitfa;     /* the last DOS statement's unit, for DS (0: none yet) */
static unsigned char cmd[64];

static void kerr(unsigned char e) {
    _bas_n = e ? e : 30;        /* (0: BREAK, as BASIC's erexit) */
    _bas_error();
}

/* The DOS statements (b65.src: dosprs, sendp's tabld, trans): the
 * parameters basic45 parsed (_bas_dpar: the ROM's parsts, _bas_dflx: bit 0
 * U, 1 U after TO, 2 ON U, 3 L, 5 I( ), 6 R, 7 I; their values) checked as
 * the ROM checks them, into dosfa / dosun (the units), ds1 / ds2 (the
 * drives), dosla, the names */
unsigned char _bas_dpar, _bas_dflx, _bas_derr, _bas_dd1, _bas_dd2, _bas_du1, _bas_du2, _bas_donu;
unsigned char _bas_did0, _bas_did1, _bas_daux, _bas_dbnk;
unsigned long _bas_dofl, _bas_dofh;
extern unsigned char _bas_bankx;
extern unsigned char _bas_chan;       /* the output channel (CMD; basic_rt.s45) */      /* BANK (basic_rt.s45: the bank XOR 128) */
unsigned char *_bas_sid;
extern unsigned char *_bas_sb;

static unsigned char dosun, ds1, ds2, dosla, dossa, dossa_t, did[2], *fn1, fn1l, *fn2, fn2l, par;
static unsigned int recno;

/* a unit (the ROM's unit: 1 is the default drive, 4-30) */
static unsigned char chkunit(unsigned char u) {
    if (u > 30) kerr(9);
    if (u == 1) return DEFDRV;
    if (u < 4) kerr(9);
    return u;
}

/* a file name (newnam): not empty, "@" first: replace */
static unsigned char *newnam(unsigned char *s, unsigned char *len) {
    unsigned char n = s[0];
    s++;
    if (!n) kerr(8);
    if (s[0] == '@') {
        if (par & 0x80) kerr(11);
        par |= 0x80;
        s++;
        if (!--n) kerr(8);
    }
    *len = n;
    return s;
}

static void dosprs(unsigned char copy) {
    par = _bas_dpar;
    dosfa = dosun = DEFDRV;
    ds1 = ds2 = 0;
    dosla = 0;
    dossa = 0x6F;
    did[0] = did[1] = 0;
    fn1l = fn2l = 0;
    if (par & 4) {
        if (!_bas_dlf) kerr(14);
        dosla = _bas_dlf;
    }
    if (_bas_dflx & 1) dosfa = dosun = chkunit(_bas_du1);
    if (_bas_dflx & 4) chkunit(_bas_donu);
    if (par & 0x10) {
        if (_bas_dd1 > 9) kerr(14);
        ds1 = ds2 = _bas_dd1;
    }
    if ((_bas_dflx & 8) && (_bas_drcl == 0 || _bas_drcl == 255)) kerr(14);
    if (_bas_dflx & 0x20) {
        if (_bas_sid[0] != 2) kerr(14);
        did[0] = _bas_sid[1];
        did[1] = _bas_sid[2];
    } else if (_bas_dflx & 0x80) {
        did[0] = _bas_did0;
        did[1] = _bas_did1;
    }
    if (par & 1) fn1 = newnam(_bas_sa, &fn1l);
    if (par & 0x20) {
        if (_bas_dd2 > 9) kerr(14);
        ds2 = _bas_dd2;
    }
    if (_bas_dflx & 2) {
        unsigned char u = chkunit(_bas_du2);
        dosun = u;
        if (!(_bas_dflx & 5) && !copy) dosfa = u;
    }
    if (par & 2) fn2 = newnam(_bas_sb, &fn2l);
    if (par & _bas_derr & 0x80) kerr(11);
    unitfa = dosfa;
}

/* the ROM's command strings (tabld; escapes: $C2 the record's secondary
 * address, $D0 the id, $D1 / $D2 drive 1 / 2, $E0 S or the record length,
 * $E1 W or L, $E2 the record number, $F0 "@", $F1 / $F2 name 1 / 2; an empty
 * name takes the character before it away). (PETSCII codes: cc45's 'W' is
 * the shifted letter.) */
static const unsigned char t_fopn[] = {0xF0, 0xD1, ':', 0xF1, ',', 0xE1, ',', 0xE0};
static const unsigned char t_dopn[] = {0xF1};
static const unsigned char t_fapn[] = {0xD1, ':', 0xF1, ',', 0x41};
static const unsigned char t_finit[] = {0x49, 0xD1};
static const unsigned char t_fcoll[] = {0x56, 0xD1};
static const unsigned char t_fconc[] = {0x43, 0xD2, ':', 0xF2, '=', 0xD2, ':', 0xF2, ',', 0xD1, ':', 0xF1};
static const unsigned char t_fhed[] = {0x4E, 0xD1, ':', 0xF1, ',', 0xD0};
static const unsigned char t_fbak[] = {0x44, 0xD2, '=', 0xD1};
static const unsigned char t_fcopy[] = {0x43, 0xD2, ':', 0xF2, '=', 0xD1, ':', 0xF1};
static const unsigned char t_fren[] = {0x52, 0xD1, ':', 0xF2, '=', 0xD1, ':', 0xF1};
static const unsigned char t_fscr[] = {0x53, 0xD1, ':', 0xF0, 0xF1};
static const unsigned char t_frscr[] = {0x46, '-', 0x52, 0xD1, ':', 0xF0, 0xF1};
static const unsigned char t_frec[] = {0x50, 0xC2, 0xE2, 0xE0};
static const unsigned char t_fdir[] = {'$', 0xD1, ':', 0xF1};
static const unsigned char t_fopnseq[] = {0xF0, 0xD1, ':', 0xF1, ',', 0x53};           /* @0:name,S */
static const unsigned char t_flock[] = {0x46, '-', 0x4C, 0xD1, ':', 0xF1};              /* F-L0:name */
static const unsigned char t_funlo[] = {0x46, '-', 0x55, 0xD1, ':', 0xF1};              /* F-U0:name */
static const unsigned char t_fmkdir[] = {'/', 0xD1, ':', 0xF1, ',', 0xE0};             /* /0:name,tracks */
static const unsigned char t_fchdir[] = {'/', 0xD1, ':', 0xF1};
static const unsigned char t_fdisk[] = {0xF1};
static const unsigned char t_fdirr[] = {0x5F, 0xD1, ':', 0xF1};

static unsigned char sendp(const unsigned char *t, unsigned char count) {
    unsigned char n = 0, k, c, rcl = _bas_drcl;
    while (count--) {
        c = *t++;
        switch (c) {
        case 0xC2: cmd[n++] = dossa_t; break;
        case 0xD0: cmd[n++] = did[0]; cmd[n++] = did[1]; break;
        case 0xD1: cmd[n++] = '0' + ds1; break;
        case 0xD2: cmd[n++] = '0' + ds2; break;
        case 0xE0: cmd[n++] = rcl == 1 ? 0 : rcl; break;
        case 0xE1:
            if (rcl) cmd[n++] = 0x4C;           /* L */
            else { rcl = 0x53; cmd[n++] = 0x57; }  /* W (then S) */
            break;
        case 0xE2: cmd[n++] = (unsigned char)recno; cmd[n++] = (unsigned char)(recno >> 8); break;
        case 0xF0: if (par & 0x80) cmd[n++] = '@'; break;
        case 0xF1:
            if (!fn1l) { n--; break; }
            for (k = 0; k < fn1l && n < sizeof cmd; k++) cmd[n++] = fn1[k];
            break;
        case 0xF2:
            if (!fn2l) { n--; break; }
            for (k = 0; k < fn2l && n < sizeof cmd; k++) cmd[n++] = fn2[k];
            break;
        default: cmd[n++] = c;
        }
    }
    return n;
}

/* a command string to the drive (trans): OPEN on the command channel, a
 * special CLOSE */
static void trans(const unsigned char *t, unsigned char count) {
    unsigned char n = sendp(t, count);
    cbm_setnam((char *)cmd, n);
    cbm_setlfs(dosla, dosfa, dossa);
    cbm_clrchn();
    cbm_setbnk(0, 0);
    cbm_open();
    __jsr(0xFFC3, .A = dosla, .Carry = 1);
}

static void dsread(void);
extern void _bas_hyv(void);
/* the file name for the hypervisor (setnam_hdos): at $0400 (BASIC's hdosfn),
 * 0 after it */
static void hdosname(void) {
    volatile unsigned char *h = (volatile unsigned char *)0x0400;
    unsigned char k;
    for (k = 0; k < fn1l; k++) h[k] = fn1[k];
    h[k] = 0;
    __jsr_raw(_bas_hyv, .A = 0x2E, .X = 0x00, .Y = 0x04);
}
static void clall(void) { __jsr(0xFFE7); }
static void closeall(void) { __jsr(0xFF50, .A = dosfa); }

/* the internal drives' units (CBDOS: bank 1, $0113 / $0114) */
#define CBDOS0 (*(volatile unsigned char __far *)0x10113UL)
#define CBDOS1 (*(volatile unsigned char __far *)0x10114UL)

/* LOADIFF / SAVEIFF's file closed (also before their errors) */
void _bas_iffclose(void) {
    cbm_clrchn();
    __jsr(0xFFC3, .A = dosla, .Carry = 0);
}

void _bas_dos(void) {
    unsigned char e, sa, n;
    switch ((unsigned char)_bas_m) {
    case 1:     /* DOPEN */
    case 2:     /* APPEND */
        if (_bas_m == 1) ds[0] = 0;
        dosprs(0);
        for (sa = 0x62; ; sa++) {               /* find_sa */
            if (sa == 0x6F) kerr(1);
            if (__jsr(0xFF62, .Y = sa).Carry) break;
        }
        if (_bas_m == 1) n = sendp(t_fopn, par & 0x40 ? 8 : 4);
        else n = sendp(t_fapn, 5);
        cbm_setnam((char *)cmd, n);
        cbm_setlfs(dosla, dosfa, sa);
        cbm_clrchn();
        cbm_setbnk(0, 0);
        if (__jsr(0xFFC0, .A -> e).Carry) kerr(e);
        break;
    case 17:    /* LOADIFF, SAVEIFF: the file opened (as DLOAD / DSAVE: sa 0 / 1), the */
    case 18: {  /* channel in / out; __bas_gloadiff / gsaveiff, then 19 */
        unsigned char la = 0;
        dosprs(0);
        dossa = (unsigned char)(_bas_m - 17);
        do {                                    /* find_la */
            if (++la & 0x80) kerr(1);
        } while (!__jsr(0xFF5F, .A = la).Carry);
        dosla = la;
        n = sendp(t_fopn, 4);
        cbm_clrchn();
        cbm_setbnk(0, 0);
        cbm_setnam((char *)cmd, n);
        cbm_setlfs(dosla, dosfa, dossa);
        if (__jsr(0xFFC0, .A -> e).Carry) kerr(e);
        if (_bas_m == 17) {
            if (__jsr(0xFFC6, .X = dosla, .A -> e).Carry) kerr(e);
        } else if (__jsr(0xFFC9, .X = dosla, .A -> e).Carry) kerr(e);
        break;
    }
    case 20: {  /* KEY LOAD / SPRITE LOAD (GetLoadChannel, LoadBlock): pages of
                 * the file at page _bas_n (the keys' $10, the sprites' $06), _bas_n
                 * >> 8 of them (the next only after a full one) */
        unsigned char la = 0, k = 0, st = 0, pages = (unsigned char)((unsigned int)_bas_n >> 8);
        volatile unsigned char *t = (volatile unsigned char *)((unsigned int)(unsigned char)_bas_n << 8);
        dosprs(0);
        dossa = 0;
        do {
            if (++la & 0x80) kerr(1);
        } while (!__jsr(0xFF5F, .A = la).Carry);
        dosla = la;
        n = sendp(t_fopn, 4);
        cbm_clrchn();
        cbm_setbnk(0, 0);
        cbm_setnam((char *)cmd, n);
        cbm_setlfs(dosla, dosfa, dossa);
        if (__jsr(0xFFC0, .A -> e).Carry) kerr(e);
        if (__jsr(0xFFC6, .X = dosla, .A -> e).Carry) { _bas_iffclose(); kerr(e); }
        cbm_k_basin();                      /* (the load address) */
        cbm_k_basin();
        if (cbm_readst()) { _bas_iffclose(); kerr(29); }
        while (pages-- && !st) {
            do {
                t[k] = cbm_k_basin();
                st = (unsigned char)cbm_readst();
                if (st) break;
            } while (++k);
            t += 256;
        }
        _bas_iffclose();
        break;
    }
    case 21: {  /* TYPE name (open_SEQ_file): its characters printed until its
                 * end (STOP ends it; P: the screen cleared, [MORE] at the bottom) */
        unsigned char la = 0, sa, c, st, page = (_bas_daux & 0x80) && !_bas_chan;
        dosprs(0);
        do {
            if (++la & 0x80) kerr(1);
        } while (!__jsr(0xFF5F, .A = la).Carry);
        dosla = la;
        for (sa = 0x62; ; sa++) {
            if (sa == 0x6F) kerr(1);
            if (__jsr(0xFF62, .Y = sa).Carry) break;
        }
        dossa = sa;
        n = sendp(t_fopnseq, 6);
        cbm_clrchn();
        cbm_setbnk(0, 0);
        cbm_setnam((char *)cmd, n);
        cbm_setlfs(dosla, dosfa, dossa);
        if (__jsr(0xFFC0, .A -> e).Carry) kerr(e);
        *(volatile unsigned char __far *)0x90UL = 0;
        if (!__jsr(0xFFC6, .X = dosla).Carry) {
            if (page) _bas_putc(147);
            for (;;) {
                if (__jsr(0xFFE1).Zero) break;          /* STOP */
                c = cbm_k_basin();
                _bas_putc(c);
                if (page && (unsigned char)(cbm_wherexy() >> 8) == 24) {
                    static const unsigned char more[] = {18, '[', 0x4D, 0x4F, 0x52, 0x45, ']', 146, 27, 0x4A, 0};
                    volatile unsigned char *q = (volatile unsigned char *)0xD610;
                    unsigned char k;
                    for (k = 0; more[k]; k++) _bas_putc(more[k]);
                    while (!*q) {}
                    c = *(volatile unsigned char *)0xD619;
                    *q = c;
                    _bas_putc(27);
                    _bas_putc(0x44);
                    if (c == 3 || c == 0x51) break;
                    _bas_putc(147);
                }
                st = (unsigned char)cbm_readst();
                if (st) break;
            }
        }
        cbm_clrchn();
        _bas_putc(13);                          /* (list_done: the line flushed) */
        __jsr(0xFFC3, .A = dosla, .Carry = 0);
        break;
    }
    case 22:    /* DISK / @ "command" (the command channel, la 0); none: the
                 * drive's status printed (Print_DS) */
        dosprs(0);
        if (fn1l) {
            unsigned char c;
            n = sendp(t_fdisk, 1);
            cbm_setnam((char *)cmd, n);
            cbm_setlfs(0, dosfa, 0x6F);
            cbm_clrchn();
            cbm_setbnk(0, 0);
            c = __jsr(0xFFC0, .A -> e).Carry;
            __jsr(0xFFC3, .A = 0, .Carry = 1);
            if (c) kerr(e);
        } else {
            unsigned char k;
            unitfa = dosfa;
            dsread();
            for (k = 1; k <= ds[0]; k++) _bas_putc(ds[k]);
        }
        break;
    case 23:    /* LOCK name */
    case 24:    /* UNLOCK name */
        dosprs(0);
        trans(_bas_m == 23 ? t_flock : t_funlo, 6);
        break;
    case 25:    /* MKDIR name [, L tracks] (CBDOS units only; 3 tracks at least) */
        dosprs(0);
        if (dosfa != CBDOS0 && dosfa != CBDOS1) kerr(44);
        if (_bas_drcl <= 3) _bas_drcl = 3;
        trans(t_fmkdir, 6);
        break;
    case 26:    /* CHDIR name ("/" alone: up to the root); on the SD card (unit
                 * 12) through the hypervisor */
        dosprs(0);
        if (dosfa == 12) {
            ds[0] = 0;
            hdosname();
            if (!__jsr_raw(_bas_hyv, .A = 0x34).Carry) kerr(4);
            __jsr_raw(_bas_hyv, .A = 0x0C);
            break;
        }
        trans(t_fchdir, fn1l == 1 && fn1[0] == '/' ? 1 : 4);
        break;
    case 27:    /* SET DEF unit: the default drive and the DOS unit */
        DEFDRV = (unsigned char)_bas_n;
        dosfa = (unsigned char)_bas_n;
        ds[0] = 0;
        break;
    case 28: {  /* SET DISK unit [TO new]: the DOS unit; with a new number the
                 * drive renumbered (M-W 119 0 2, files closed) */
        unsigned char u = (unsigned char)_bas_n, nu = (unsigned char)(_bas_n >> 8);
        dosfa = u;
        if (nu) {
            unsigned char c[8] = {0x4D, '-', 0x57, 119, 0, 2, 0, 0};
            closeall();
            c[6] = nu | 32;
            c[7] = nu | 64;
            cbm_setnam((char *)c, 8);
            cbm_setbnk(0, 0);
            cbm_clrchn();
            cbm_setlfs(0, dosfa, 0x6F);
            cbm_open();
            __jsr(0xFFC3, .A = 0, .Carry = 1);
            dosfa = nu;
            KFA = nu;
        }
        ds[0] = 0;
        break;
    }
    case 29: {  /* SET VERIFY ON / OFF (_bas_n 1 / 0): U0>V1 / U0>V0 */
        unsigned char c[5] = {0x55, '0', '>', 0x56, 0};
        c[4] = '0' + (unsigned char)_bas_n;
        if (!dosfa) dosfa = DEFDRV;
        cbm_setnam((char *)c, 5);
        cbm_setbnk(0, 0);
        cbm_clrchn();
        cbm_setlfs(0, dosfa, 0x6F);
        cbm_open();
        __jsr(0xFFC3, .A = 0, .Carry = 1);
        ds[0] = 0;
        break;
    }
    case 30: {  /* MOUNT name [, U unit] (the hypervisor: the image on the SD
                 * card as unit 8 or 9; not found: ?FILE NOT FOUND); MOUNT
                 * alone: the real drive selected, its head stepped home and
                 * back (the freezer's disk select) */
        volatile unsigned char *fdc = (volatile unsigned char *)0xD080;
        dosprs(0);
        if (!(par & 1)) {
            unsigned char tr;
            *(volatile unsigned char *)0xD68B &= ~1;
            *(volatile unsigned char *)0xD6A1 |= 1;
            fdc[0] = 0x60;
            fdc[1] = 0x20;
            while (!(fdc[2] & 1)) {
                fdc[1] = 0x10;
                while (fdc[2] & 0x80) {}
            }
            tr = fdc[4];
            while (tr) {
                fdc[1] = 0x18;
                while (fdc[2] & 0x80) {}
                tr--;
            }
            fdc[0] = 0;
            *(volatile unsigned char *)0xD6A1 |= 1;
            break;
        }
        hdosname();
        if (!__jsr_raw(_bas_hyv, .A = dosfa == 9 ? 0x46 : 0x40, .X = dosfa).Carry) kerr(4);
        break;
    }
    case 19:    /* exit_GKI_disk_op: closed; STOP down: BREAK */
        _bas_iffclose();
        if (__jsr(0xFFE1).Zero) {
            while (__jsr(0xFFE1).Zero) {}
            kerr(0);
        }
        break;
    case 3:     /* DCLOSE */
        dosprs(0);
        if (par & 4) {
            if (__jsr(0xFFC3, .A = dosla, .Carry = 0, .A -> e).Carry) kerr(e);
        } else closeall();
        break;
    case 5:     /* HEADER */
        dosprs(0);
        clall();
        trans(t_fhed, did[0] ? 6 : 4);
        break;
    case 6:     /* SCRATCH (ERASE, DELETE "name"): R recovers */
        dosprs(0);
        if (_bas_dflx & 0x40) trans(t_frscr, 7);
        else trans(t_fscr, 5);
        break;
    case 7: dosprs(0); trans(t_fren, 8); break;          /* RENAME */
    case 8: dosprs(0); trans(t_fconc, 12); break;        /* CONCAT */
    case 9:     /* COLLECT */
        dosprs(0);
        clall();
        trans(t_fcoll, par & 0x10 ? 2 : 1);
        break;
    case 10:    /* DCLEAR */
        dosprs(0);
        trans(t_finit, 2);
        closeall();
        break;
    case 11:    /* BACKUP: D TO D, or U TO U (the units as drives 0 / 1) */
        dosprs(0);
        if ((par & 0x30) != 0x30) {
            ds1 = dosfa - 8;
            if (dosun == dosfa) kerr(11);
            ds2 = dosun - 8;
        }
        closeall();
        trans(t_fbak, 4);
        break;
    case 13:    /* BLOAD, BVERIFY (bload_boot): at P or the file's address */
    case 14: {
        unsigned char bnk = _bas_daux & 1 ? _bas_dbnk : _bas_bankx ^ 128, a, y, st, lo, hi;
        unsigned long at = _bas_daux & 2 ? _bas_dofl : 0xFFFFUL;
        dosprs(0);
        n = sendp(t_fopn, 4);
        cbm_setnam((char *)cmd, n);
        cbm_setlfs(0, dosfa, (unsigned char)at == 0xFF && (unsigned char)(at >> 8) == 0xFF ? 0xFF : 0);
        /* (setbank_from_dos_args: a 16-bit address in bank B (BANK), else its bank) */
        if (at >> 16) { a = (unsigned char)(at >> 24) | 0x80; y = (unsigned char)(at >> 16); }
        else { a = bnk & 0x7F; y = 0; }
        __jsr(0xFF6B, .A = a, .X = 0, .Y = y);
        if (__jsr(0xFFD5, .A = (_bas_m == 14 ? 1 : 0) | (_bas_dflx & 0x40), .X = (unsigned char)at,
                  .Y = (unsigned char)(at >> 8), .A -> e, .X -> lo, .Y -> hi).Carry) kerr(e);
        st = (unsigned char)cbm_readst();
        cbm_setbnk(0, 0);
        if (_bas_m == 14) {
            if (st & 0x10) {
                /* VERIFY ERROR AT $ the address (8 hex digits, up to 4 leading zeros left out) */
                static const unsigned char msg[] = {13, 0x56, 0x45, 0x52, 0x49, 0x46, 0x59, ' ', 0x45, 0x52, 0x52, 0x4F,
                                                    0x52, ' ', 0x41, 0x54, ' ', '$', 0};
                unsigned long v = (at >> 16) << 16 | lo | (unsigned)hi << 8;
                unsigned char k, d, lead = 1;
                for (k = 0; msg[k]; k++) _bas_putc(msg[k]);
                for (k = 0; k < 8; k++) {
                    d = (unsigned char)(v >> 28) & 15;
                    v <<= 4;
                    if (lead && !d && k < 4) continue;
                    lead = 0;
                    _bas_putc(d < 10 ? '0' + d : 0x41 + d - 10);
                }
            }
        } else if (st & 0xBF) kerr(29);             /* ?LOAD ERROR */
        break;
    }
    case 15: {  /* BSAVE: P start TO P end (end not saved), as KERNAL SAVE writes it */
        unsigned char bnk = _bas_daux & 1 ? _bas_dbnk : _bas_bankx ^ 128;
        unsigned long p = _bas_dofl, end = _bas_dofh;
        unsigned char __far *q;
        dosprs(0);
        if (p >= end) kerr(14);
        n = sendp(t_fopn, 4);
        if (!(p >> 16)) { p |= (unsigned long)(bnk & 0x7F) << 16; end |= (unsigned long)(bnk & 0x7F) << 16; }
        cbm_setnam((char *)cmd, n);
        cbm_setlfs(0, dosfa, 1);
        cbm_setbnk(0, 0);
        if (__jsr(0xFFC0, .A -> e).Carry) kerr(e);
        if (__jsr(0xFFC9, .X = 0, .A -> e).Carry) { __jsr(0xFFC3, .A = 0, .Carry = 0); kerr(e); }
        cbm_k_bsout((unsigned char)_bas_dofl);
        cbm_k_bsout((unsigned char)(_bas_dofl >> 8));
        for (q = (unsigned char __far *)p; p < end; p++) cbm_k_bsout(*q++);
        cbm_clrchn();
        __jsr(0xFFC3, .A = 0, .Carry = 0);
        break;
    }
    case 16: {  /* DIR / CATALOG */
        static unsigned char line[64];
        unsigned char wide, page, k, x, col, row, c, y;
        int integr = -2;
        unsigned int blocks;
        dosprs(0);
        page = _bas_daux & 0x80 && !_bas_chan;
        if (page) _bas_putc(147);
        x = 1;
        if (par & 0x11) x = par & 1 ? 4 : 2;
        n = sendp(_bas_dflx & 0x40 ? t_fdirr : t_fdir, x);
        cbm_setnam((char *)cmd, n);
        cbm_setbnk(0, 0);
        cbm_setlfs(0, dosfa, 0x60);
        cbm_clrchn();
        if (__jsr(0xFFC0, .A -> e).Carry) {
            cbm_clrchn();
            __jsr(0xFFC3, .A = 0, .Carry = 0);
            if (_bas_chan) __jsr(0xFFC9, .X = _bas_chan);
            kerr(e);
        }
        wide = (par & 0x40) && !_bas_chan && !page;
        y = 3;
        for (;;) {
            if (wide) {
                /* dirpos: the next column after 21 rows; past the screen: a key, then again */
                for (;;) {
                    integr++;
                    if (integr == -1) { _bas_putc(147); break; }
                    row = (unsigned char)integr;
                    col = 0;
                    while (row >= 21) { row -= 21; col += 27; }
                    if (col < 80) { __jsr(0xFFF0, .Carry = 0, .X = row + 1, .Y = col); break; }
                    while (!cbm_k_getin()) {}
                    integr = -2;
                }
            }
            if (__jsr(0xFFC6, .X = 0).Carry) break;
            do {
                if (cbm_readst()) goto done;
                blocks = cbm_k_basin();
                blocks |= cbm_k_basin() << 8;
            } while (--y);
            k = 0;
            do c = cbm_k_basin(); while (c == ' ');
            for (;;) {
                line[k] = c;
                if (!c) break;
                if (k < sizeof line - 1) k++;
                if (cbm_readst()) goto done;
                c = cbm_k_basin();
            }
            if (wide) {
                line[20] = 0;
                if (line[0] == 0x42) __jsr(0xFFF0, .Carry = 0, .X = 22, .Y = 0);    /* B(LOCKS FREE) */
            }
            cbm_clrchn();
            if (_bas_chan) __jsr(0xFFC9, .X = _bas_chan);
            {
                /* the blocks, right-aligned in 4 */
                unsigned char dg[6], m = 0;
                do { dg[m++] = '0' + blocks % 10; blocks /= 10; } while (blocks);
                for (k = m; k < 4; k++) _bas_putc(' ');
                while (m) _bas_putc(dg[--m]);
            }
            _bas_putc(' ');
            for (k = 0; line[k]; k++) _bas_putc(line[k]);
            _bas_putc(13);
            if (_bas_chan & 0x80) _bas_putc(10);
            cbm_clrchn();
            if (__jsr(0xFFE1).Zero) break;         /* STOP */
            if (page && (unsigned char)(cbm_wherexy() >> 8) == 24) {
                /* [MORE], a key from the typing queue ($D610) */
                static const unsigned char more[] = {18, '[', 0x4D, 0x4F, 0x52, 0x45, ']', 146, 27, 0x4A, 0};
                volatile unsigned char *q = (volatile unsigned char *)0xD610;
                for (k = 0; more[k]; k++) _bas_putc(more[k]);
                while (!*q) {}
                c = *(volatile unsigned char *)0xD619;   /* (its PETSCII; a write takes it off) */
                *q = c;
                _bas_putc(27);
                _bas_putc(0x44);
                if (c == 3 || c == 0x51) break;
                _bas_putc(147);
            }
            y = 2;
        }
    done:
        if (wide) __jsr(0xFFF0, .Carry = 0, .X = 22, .Y = 0);
        cbm_clrchn();
        if (__jsr(0xFFC3, .A = 0, .Carry = 0, .A -> e).Carry) kerr(e);
        if (_bas_chan) __jsr(0xFFC9, .X = _bas_chan);
        break;
    }
    case 12: {  /* COPY: on one unit by its DOS (the internal drives' units as drives 0 / 1) */
        unsigned char c0 = CBDOS0, c1 = CBDOS1, iec = 0;
        static const unsigned char star[] = {1, '*'};
        dosprs(1);
        if (dosfa != c0) { if (dosfa == c1) ds1++; else iec = 1; }
        if (!iec) {
            if (dosun != c0) { if (dosun == c1) ds2++; else iec = 1; }
            if (!iec) { par |= 0x30; dosun = dosfa; }
        }
        if ((par & 0xC7) && (par & 3) != 3) { fn2 = (unsigned char *)star + 1; fn2l = 1; par |= 2; }
        if (dosfa != dosun) kerr(9);             /* (a copy between IEC units: not compiled) */
        trans(t_fcopy, 8);
        break;
    }
    }
}

/* RECORD# __bas_dlf, __bas_n [, __bas_drcl]: the file's position (P on the
 * command channel of its unit) */
void _bas_record(void) {
    unsigned char fa, sa;
    if (!_bas_dlf || !_bas_drcl || _bas_drcl == 255) kerr(14);
    if (__jsr(0xFF5F, .A = _bas_dlf, .X -> fa, .Y -> sa).Carry) kerr(3);
    par = 0;
    dossa_t = sa;
    dosfa = unitfa = fa;
    dosla = 0;
    dossa = 0x6F;
    recno = (unsigned)_bas_n;
    trans(t_frec, 4);
}

/* DS$ read from the drive's error channel (Read_DS) */
static void dsread(void) {
    unsigned char u = KFA, c, n = 0, e;
    if (u < 8) u = unitfa ? unitfa : DEFDRV;
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

/* CLR DS (Clear_DS): DS$ forgotten (the drive's status read again) */
void _bas_dsclr(void) { ds[0] = 0; }

/* CLR ST (Clear_ST): the KERNAL's I/O status ($90) 0 */
void _bas_stclr(void) { *(volatile unsigned char __far *)0x90UL = 0; }

/* CLR's files: all closed (the KERNAL's CLALL) */
void _bas_clall(void) { clall(); }

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
        /* DLOAD (dosprs, dload_boot: "$" a directory, else fopn's "0:name") */
        unsigned char m;
        dosprs(0);
        _bas_cdev = dosfa;
        _bas_csa = 0;
        m = fn1[0] == '$' ? sendp(t_dopn, 1) : sendp(t_fopn, 4);
        for (k = 0; k < m && n < 31; k++) out[n++] = cmd[k];
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

extern unsigned long _bas_fa;
extern void _bas_chkin(void);
extern void _bas_chkout(void);
extern void _bas_clrch(void);

void _bas_fread(void) {
    unsigned char __far *p = (unsigned char __far *)_bas_fa;
    unsigned int n = (unsigned)_bas_m;
    _bas_n = _bas_dlf;
    _bas_chkin();
    do *p++ = cbm_k_getin(); while (--n);
    _bas_clrch();
}

void _bas_fwrite(void) {
    unsigned char __far *p = (unsigned char __far *)_bas_fa;
    unsigned int n = (unsigned)_bas_m;
    _bas_n = _bas_dlf;
    _bas_chkout();
    do cbm_k_bsout(*p++); while (--n);
    _bas_clrch();
}
