/* basic_in.c — INPUT, GET, READ, TI of BASIC 65 programs compiled by basic45 --compile (#369)
 *
 * As BASIC 65's (b65.src: input, inpcon, get):
 *   _bas_iline: a line from the keyboard (KERNAL BASIN: the screen editor),
 *     "? " first when __bas_n is 1, a carriage return after it. __bas_n := 1
 *     for an empty line (the INPUT ends, its variables unchanged), else 0.
 *   _bas_iitem (INPUT's items), _bas_read, _bas_restore (READ's DATA items):
 *     basic_items.s45, in assembly; the line in _bas_ibuf from _bas_ipos, the
 *     line used up: _bas_morein ("?? ", another), bad data: _bas_iredo.
 *     Input left over is ignored (as BASIC 65: no ?EXTRA IGNORED).
 *   _bas_get: a key (KERNAL GETIN): __bas_n := its code, 0 for none;
 *     __bas_m bit 0 (GETKEY): waits for one; bit 1: also the string of it
 *     ("" for none) into __bas_sr's buffer.
 *   _bas_ti: TI into __float_a: CIA 2's timers A and B (one 32-bit count,
 *     started if they do not run) in seconds, as BASIC 65's Get_TI_CIA.
 *   _bas_tis: TI$ ("hh:mm:ss", the RTC) into __bas_sr's buffer.
 * __bas_m 5 (LINE INPUT): the rest of the line as it is. _bas_lin: LINE
 * INPUT#'s line (the ROM's linputn) into __bas_sr's buffer.
 * INPUT# / GET# (__bas_chan, a channel CHKIN made the input): no prompts, no
 * echoed carriage return, a line as BASIC's InputBuffer, bad data ?FILE DATA
 * instead of ?REDO FROM START, a bad status (ST AND $87) ends the INPUT#;
 * GET# never waits, a 0 is CHR$(0).
 * Numbers are read as strtof reads them (basic_fin.s45; correctly rounded: the ROM's own
 * conversion can differ in the last bit).
 */
#include <cbm.h>

extern unsigned char *_bas_sr;
extern int _bas_n, _bas_m;
extern unsigned char _float_a[5];
extern unsigned char _bas_data[];
extern unsigned char _bas_chan;      /* INPUT# / GET#: the channel (basic_rt.s45) */
extern void _bas_error(void);

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

#define LINE 160
#define NUMLEN 80     /* a number item: its first 80 characters (basic_fin.s45) */
/* the line INPUT reads and its position (basic_items.s45's items too) */
unsigned char _bas_ibuf[LINE + 1];
unsigned char _bas_ipos;
#define line _bas_ibuf
#define pos _bas_ipos

static void out(const char *s) {
    while (*s) cbm_k_bsout((unsigned char)*s++);
}

/* a character from the channel (BASIC's inchr: the KERNAL's error raised) */
static unsigned char basin(void) {
    unsigned char c;
    if (__jsr(0xFFCF, .A -> c).Carry) raise(c ? c : 30);
    return c;
}

static void readline(void) {
    unsigned char n = 0, c;
    if (_bas_chan) {
        /* INPUT#: as BASIC's InputBuffer: a line feed first skipped, up to a
         * carriage return, line feed or 0, at most 160 characters (the rest
         * the next line's); no carriage return echoed */
        c = basin();
        if (c == 10) c = basin();
        while (c != 13 && c != 10 && c != 0) {
            line[n++] = c;
            if (n == LINE) break;
            c = basin();
        }
        line[n] = 0;
        pos = 0;
        return;
    }
    for (;;) {
        c = cbm_k_basin();
        if (c == 13 || c == 10 || c == 0) break;
        if (n < LINE) line[n++] = c;
    }
    line[n] = 0;
    pos = 0;
    cbm_k_bsout(13);
}

void _bas_iline(void) {
    if (_bas_n && !_bas_chan) out("? ");
    readline();
    /* INPUT#: a bad status (EOF is not) ends it, as an empty line */
    if (_bas_chan && (cbm_readst() & 0x87)) { _bas_n = 1; return; }
    _bas_n = line[0] == 0;
}


/* LINE INPUT# (the ROM's linputn): a line (InputBuffer) into __bas_sr's buffer */
void _bas_lin(void) {
    unsigned char k, n;
    unsigned char *r = _bas_sr;
    readline();
    for (n = 0; line[n]; n++) {}
    r[0] = n;
    for (k = 0; k < n; k++) r[k + 1] = line[k];
}

/* the line used up: another ("?? " first, from the keyboard) (basic_items.s45) */
void _bas_morein(void) {
    if (!_bas_chan) out("?? ");
    readline();
}

/* bad data for an INPUT item: ?REDO FROM START (__bas_n 1: the INPUT again) */
void _bas_iredo(void) {
    if (_bas_chan) raise(24);    /* INPUT#: ?FILE DATA */
    out("?redo from start");     /* (lower case: PETSCII's unshifted letters) */
    cbm_k_bsout(13);
    _bas_n = 1;
}

void _bas_get(void) {
    unsigned char c;
    do c = cbm_k_getin(); while (!c && (_bas_m & 1) && !_bas_chan);
    _bas_n = c;
    if (_bas_m & 2) {
        _bas_sr[0] = c != 0 || _bas_chan;    /* (GET#: CHR$(0) too) */
        _bas_sr[1] = c;
    }
}

#define CIA2 ((volatile unsigned char *)0xDD00)
#define VIC_PAL (*(volatile unsigned char *)0xD06F)     /* bit 7: NTSC */

void _bas_ti(void) {
    unsigned char t[4], k;
    unsigned long n;
    float f;
    /* count phi2, start the timers (TOD 50 Hz on a PAL machine) */
    CIA2[0x0E] = ((VIC_PAL & 0x80) ^ 0x80) | 0x41;
    CIA2[0x0F] = 0x41;
    /* (not while timer A is about to underflow: its bits 7 and 6 clear) */
    while (!(CIA2[4] & 0xC0)) {}
    for (k = 0; k < 4; k++) t[k] = CIA2[4 + k];
    n = 0xFFFFFFFFUL - (t[0] | (unsigned)t[1] << 8 | (unsigned long)t[2] << 16 | (unsigned long)t[3] << 24);
    /* the count / 2^20, times the ROM's scale */
    f = (float)n / 1048576.0;
    f = f * (VIC_PAL & 0x80 ? 1.057777 : 1.066890);
    for (k = 0; k < 5; k++) _float_a[k] = ((unsigned char *)&f)[k];
}

/* CLR TI (b65.src Clear_TI): timers A and B stopped, loaded with $FFFF each,
 * started again */
void _bas_ticlr(void) {
    unsigned char k;
    CIA2[0x0E] = ((VIC_PAL & 0x80) ^ 0x80) | 0x40;
    CIA2[0x0F] = 0x40;
    for (k = 0; k < 4; k++) CIA2[4 + k] = 0xFF;
    CIA2[0x0E] |= 0x10;
    CIA2[0x0F] |= 0x10;
    CIA2[0x0E] = ((VIC_PAL & 0x80) ^ 0x80) | 0x41;
    CIA2[0x0F] = 0x41;
}

void _bas_tis(void) {
    unsigned char __far *rtc = (unsigned char __far *)0xFFD7110UL;
    unsigned char b[3], k, i;
    unsigned char *r = _bas_sr;
    /* seconds, minutes, hours (BCD), each read until it holds */
    for (k = 0; k < 3; k++) {
        do b[k] = rtc[k]; while (b[k] != rtc[k]);
    }
    r[0] = 8;
    for (k = 0, i = 1; k < 3; k++) {
        unsigned char v = b[2 - k];
        if (k) r[i++] = ':';
        r[i++] = (v >> 4) | '0';
        r[i++] = (v & 15) | '0';
    }
}

/* (READ, RESTORE and INPUT's items: basic_items.s45) */
