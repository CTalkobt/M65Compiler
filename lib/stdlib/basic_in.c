/* basic_in.c — INPUT, GET, READ, TI of BASIC 65 programs compiled by basic45 --compile (#369)
 *
 * As BASIC 65's (b65.src: input, inpcon, get):
 *   _bas_iline: a line from the keyboard (KERNAL BASIN: the screen editor),
 *     "? " first when __bas_n is 1, a carriage return after it. __bas_n := 1
 *     for an empty line (the INPUT ends, its variables unchanged), else 0.
 *   _bas_iitem: the next item of the line for a variable (__bas_m: 0 a number
 *     into __float_a, 1 a string into __bas_sr's buffer). Leading spaces are
 *     skipped; the line used up (its end or a ':') asks for another with
 *     "?? ". A string in quotes runs to the next quote, one without to the next
 *     ',' or ':'. After the item only spaces, then ',', ':' or the end may
 *     follow, else "?REDO FROM START" and __bas_n := 1 (the INPUT starts
 *     again); else __bas_n := 0. Input left over is ignored (as BASIC 65:
 *     no ?EXTRA IGNORED).
 *   _bas_get: a key (KERNAL GETIN): __bas_n := its code, 0 for none;
 *     __bas_m bit 0 (GETKEY): waits for one; bit 1: also the string of it
 *     ("" for none) into __bas_sr's buffer.
 *   _bas_read: the next DATA item for a variable (__bas_m as _bas_iitem's):
 *     items as INPUT's, from __bas_data (the DATA statements' text, basic45
 *     makes it); after the last of a statement the next statement's; none
 *     left: ?OUT OF DATA; not a number (or more after one): ?TYPE MISMATCH.
 *   _bas_restore: READ from the DATA at offset __bas_n of __bas_data.
 *   _bas_ti: TI into __float_a: CIA 2's timers A and B (one 32-bit count,
 *     started if they do not run) in seconds, as BASIC 65's Get_TI_CIA.
 *   _bas_tis: TI$ ("hh:mm:ss", the RTC) into __bas_sr's buffer.
 * __bas_m 5 (LINE INPUT): the rest of the line as it is. _bas_lin: LINE
 * INPUT#'s line (the ROM's linputn) into __bas_sr's buffer.
 * INPUT# / GET# (__bas_chan, a channel CHKIN made the input): no prompts, no
 * echoed carriage return, a line as BASIC's InputBuffer, bad data ?FILE DATA
 * instead of ?REDO FROM START, a bad status (ST AND $87) ends the INPUT#;
 * GET# never waits, a 0 is CHR$(0).
 * Numbers are read by strtof (correctly rounded; the ROM's own conversion can
 * differ in the last bit).
 */
#include <cbm.h>
#include <stdlib.h>

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
static unsigned char line[LINE + 1];
static unsigned char pos;

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

static void skipsp(void) {
    while (line[pos] == ' ') pos++;
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

void _bas_iitem(void) {
    unsigned char c, start, n;
    if (_bas_m & 4) {
        /* LINE INPUT: the rest of the line as it is (leading spaces, commas,
         * colons; count_to_eol), another line when it is used up */
        unsigned char *r = _bas_sr;
        if (!line[pos]) {
            if (!_bas_chan) out("?? ");
            readline();
        }
        n = 0;
        while (line[pos]) r[++n] = line[pos++];
        r[0] = n;
        _bas_n = 0;
        return;
    }
    skipsp();
    if (line[pos] == 0 || line[pos] == ':') {
        if (!_bas_chan) out("?? ");
        readline();
        skipsp();
    }
    if (_bas_m) {
        unsigned char *r = _bas_sr;
        if (line[pos] == '"') {
            start = ++pos;
            while (line[pos] && line[pos] != '"') pos++;
            n = pos - start;
            if (line[pos] == '"') pos++;
            skipsp();
            c = line[pos];
            if (c && c != ',' && c != ':') goto redo;
        } else {
            start = pos;
            while (line[pos] && line[pos] != ',' && line[pos] != ':') pos++;
            n = pos - start;
        }
        r[0] = n;
        for (c = 0; c < n; c++) r[c + 1] = line[start + c];
    } else {
        char *end;
        float f = strtof((char *)line + pos, &end);
        unsigned char k;
        pos = (unsigned char)((unsigned char *)end - line);
        skipsp();
        c = line[pos];
        if (c && c != ',' && c != ':') goto redo;
        for (k = 0; k < 5; k++) _float_a[k] = ((unsigned char *)&f)[k];
    }
    if (line[pos] == ',') pos++;
    _bas_n = 0;
    return;
redo:
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

static unsigned char *drec;      /* the DATA statement READ is in (0: none yet) */
static unsigned char dpos, dneed = 1;

void _bas_restore(void) {
    drec = _bas_data + _bas_n;
    dneed = 2;                   /* (2: drec itself is the next statement) */
}

void _bas_read(void) {
    unsigned char *t, len, start, n, c;
    if (!drec) { drec = _bas_data; dneed = 2; }
    if (dneed) {
        if (dneed == 1 && drec[0] != 0xFF) drec += drec[0] + 1;
        if (drec[0] == 0xFF) raise(13);
        dpos = 0;
        dneed = 0;
    }
    len = drec[0];
    t = drec + 1;
    while (dpos < len && t[dpos] == ' ') dpos++;
    if (_bas_m) {
        unsigned char *r = _bas_sr;
        if (dpos < len && t[dpos] == '"') {
            start = ++dpos;
            while (dpos < len && t[dpos] != '"') dpos++;
            n = dpos - start;
            if (dpos < len) dpos++;
            while (dpos < len && t[dpos] == ' ') dpos++;
            if (dpos < len && t[dpos] != ',') raise(22);
        } else {
            start = dpos;
            while (dpos < len && t[dpos] != ',') dpos++;
            n = dpos - start;
        }
        r[0] = n;
        for (c = 0; c < n; c++) r[c + 1] = t[start + c];
    } else {
        char item[40], *end;
        float f;
        unsigned char k = 0;
        while (dpos < len && t[dpos] != ',' && k < sizeof item - 1) item[k++] = (char)t[dpos++];
        while (k && item[k - 1] == ' ') k--;
        item[k] = 0;
        f = strtof(item, &end);
        if (*end) raise(22);
        for (k = 0; k < 5; k++) _float_a[k] = ((unsigned char *)&f)[k];
        while (dpos < len && t[dpos] != ',') dpos++;
    }
    if (dpos < len) dpos++;          /* the ',' */
    else dneed = 1;
}
