/* basic_sys.c — basic45 --compile's SYS, RREG, DMA, EDMA and FRE (#369), as
 * BASIC 65's (b65.src V920413: sys, rreg, dma, edma, fre)
 *
 * SYS calls the KERNAL's _jsr_far ($FF6E) with the registers in its base-page
 * bytes ($02 bank, $03/$04 the address, $05 S, $06-$09 A X Y Z), which it
 * updates when the routine returns. The float run time calls the ROM through
 * the same bytes, so the registers are kept here (_bas_sregs, taken from the
 * ROM's at the start: _bas_sysinit) and written there only around a SYS:
 * RREG reads them, and a SYS without a register's value passes the last one.
 * Arguments: __bas_fa (SYS's address), _bas_sysv / _bas_sysm (the registers
 * given: bit 0 A ... bit 4 S), _bas_dl (DMA / EDMA's list), _bas_m (RREG's
 * register), __float_a (FRE's argument); results in _bas_n / __float_a.
 */

#include <stdlib.h>

extern int _bas_n, _bas_m;
extern unsigned long _bas_fa;
extern unsigned char _bas_bankx;           /* BANK (basic_rt.s45: the bank XOR 128) */
extern unsigned char _float_a[5];
extern void _bas_error(void);
extern void _bas_ayint(void);
extern void _float_utof(void);

#define ROMBP(a) (*(volatile unsigned char __far *)(unsigned long)(a))
#define INIT_STATUS (*(volatile unsigned char *)0x1104)

unsigned char _bas_sregs[5];               /* A X Y Z S */
unsigned char _bas_sysv[5], _bas_sysm;

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

void _bas_sysinit(void) {
    unsigned char k;
    for (k = 0; k < 4; k++) _bas_sregs[k] = ROMBP(6 + k);
    _bas_sregs[4] = ROMBP(5);
}

/* SYS address [, a, x, y, z, s] (poke_sub: a 16-bit address in the BANK's bank) */
void _bas_sys(void) {
    unsigned char k, st, bank = (unsigned int)(_bas_fa >> 16) ? (unsigned char)(_bas_fa >> 16) : (unsigned char)(_bas_bankx ^ 128);
    for (k = 0; k < 5; k++)
        if (_bas_sysm & (1 << k)) _bas_sregs[k] = _bas_sysv[k];
    ROMBP(2) = bank;
    ROMBP(3) = (unsigned char)(_bas_fa >> 8);
    ROMBP(4) = (unsigned char)_bas_fa;
    ROMBP(5) = _bas_sregs[4];
    for (k = 0; k < 4; k++) ROMBP(6 + k) = _bas_sregs[k];
    st = INIT_STATUS;
    INIT_STATUS = st & 0xFE;                /* (BASIC's IRQ off) */
    __jsr(0xFF6E);
    INIT_STATUS = st;
    __asm__("cli");
    for (k = 0; k < 4; k++) _bas_sregs[k] = ROMBP(6 + k);
    _bas_sregs[4] = ROMBP(5);
}

/* RREG's register _bas_m (0-3 A X Y Z, 4 S) */
void _bas_rreg(void) {
    _bas_n = _bas_sregs[(unsigned char)_bas_m];
}

/* DMA / EDMA's list: cmd, count (2), source (3), target (3), sub, mod (2) */
unsigned char _bas_dl[12];
#define DMA ((volatile unsigned char *)0xD700)

/* DMA cmd, length, source, bank, target, bank [, sub, mod lo, mod hi]: one
 * job (a chained one: ?ILLEGAL QUANTITY), run from bank 0 */
void _bas_dma(void) {
    if (_bas_dl[0] & 4) raise(14);
    DMA[2] = 0;
    DMA[1] = (unsigned char)((unsigned int)_bas_dl >> 8);
    DMA[0] = (unsigned char)(unsigned int)_bas_dl;
    while (DMA[3] & 0x80) {}
}

/* EDMA: the enhanced job (the source's and target's megabytes as options) */
static unsigned char edl[18] = {0x80, 0, 0x81, 0, 0x0B, 0};
static unsigned char edmb[2];

/* an EDMA address (__bas_fa) as the list's source (_bas_m 0) or target (3) */
void _bas_edaddr(void) {
    unsigned char o = (unsigned char)_bas_m;
    _bas_dl[3 + o] = (unsigned char)_bas_fa;
    _bas_dl[4 + o] = (unsigned char)(_bas_fa >> 8);
    _bas_dl[5 + o] = (unsigned char)(_bas_fa >> 16) & 15;
    edmb[o ? 1 : 0] = (unsigned char)(_bas_fa >> 20);
}

void _bas_edma(void) {
    unsigned char k;
    edl[1] = edmb[0];
    edl[3] = edmb[1];
    for (k = 0; k < 12; k++) edl[6 + k] = _bas_dl[k];
    DMA[2] = 0;
    DMA[1] = (unsigned char)((unsigned int)edl >> 8);
    DMA[5] = (unsigned char)(unsigned int)edl;
}

/* FRE(n): a negative n the ROM's version (920413), 0 and 1 the heap's free
 * bytes (a compiled program's strings and arrays are there; it has no text
 * area of its own), 4 and 5 65535, others 0 (n > 255: ?ILLEGAL QUANTITY) */
void _bas_fre(void) {
    unsigned int r = 0;
    unsigned char n;
    if (_float_a[0] && (_float_a[1] & 0x80)) {
        _float_a[0] = 0x94;                 /* 920413 */
        _float_a[1] = 0x60;
        _float_a[2] = 0xB5;
        _float_a[3] = 0xD0;
        _float_a[4] = 0x00;
        return;
    }
    _bas_ayint();
    if ((unsigned int)_bas_n > 255) raise(14);
    n = (unsigned char)_bas_n;
    if (n < 2) r = _heapmemavail();
    else if (n == 4 || n == 5) r = 0xFFFF;
    _bas_n = r;
    __jsr_raw(_float_utof, .A = (unsigned char)r, .X = (unsigned char)(r >> 8));
}
