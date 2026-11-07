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
 * The channels (PRINT#, CMD, INPUT#, GET#) are basic_rt.s45's __bas_chkout,
 * __bas_chkin and __bas_clrch.
 */
#include <cbm.h>

extern unsigned char *_bas_sa;
extern int _bas_n;
extern void _bas_error(void);

unsigned char _bas_olf, _bas_odev, _bas_osa, _bas_ocnt;

static void kerr(unsigned char e) {
    _bas_n = e ? e : 30;        /* (0: BREAK, as BASIC's erexit) */
    _bas_error();
}

void _bas_open(void) {
    unsigned char dev, sa, e, len = 0;
    unsigned char *nm = _bas_sa;
    if (_bas_ocnt < 2) {
        dev = *(volatile unsigned char *)0x1106;
        sa = 0;
    } else {
        dev = _bas_odev;
        if (_bas_ocnt < 3) sa = dev < 4 ? 0 : 0xFF;
        else sa = _bas_osa;
    }
    if (nm) { len = nm[0]; nm++; }
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
