/* test_basic_ti_xemu.c — basic45 --compile's TI, CLR TI, TI$ and SLEEP in
 * assembly (basic_ti.s45, #369) against their former C (basic_in.c's
 * _bas_ti, _bas_ticlr, _bas_tis and basic_fn.c's _bas_sleep, copied here as
 * the reference).
 *
 * Results at $C0C0 (0 = pass each):
 *   [0] the count's conversion to seconds (__bas_tconv: the same counts,
 *       PAL and NTSC): the floats' differences
 *   [1] TI read between two readings of the reference's: out of order
 *   [2] CLR TI: the control registers and the timers after it
 *   [3] TI$: differences from the reference's (read before and after it)
 *   [4] SLEEP: too short, too long, ?ILLEGAL QUANTITY below 0
 *   [5] $AA
 */
#include <setjmp.h>
#include <string.h>

extern unsigned char *_bas_sr;
extern int _bas_n;
extern unsigned char _float_a[5];
extern unsigned char _bas_tcnt[4];
extern void _bas_ti(void), _bas_ticlr(void), _bas_tis(void), _bas_sleep(void), _bas_tconv(void);

#define OUT ((volatile unsigned char *)0xC0C0)

static jmp_buf back;
static int raised;
void _bas_error(void) { raised = _bas_n; longjmp(back, 1); }

/* --- the reference: basic_in.c / basic_fn.c as they were --- */
#define CIA2 ((volatile unsigned char *)0xDD00)
#define VIC_PAL (*(volatile unsigned char *)0xD06F)     /* bit 7: NTSC */

static float geta(void) {
    float f;
    unsigned char k;
    for (k = 0; k < 5; k++) ((unsigned char *)&f)[k] = _float_a[k];
    return f;
}
static void seta(float f) {
    unsigned char k;
    for (k = 0; k < 5; k++) _float_a[k] = ((unsigned char *)&f)[k];
}

/* (_bas_ti's conversion of n, as it was) */
static void ref_conv(unsigned long n) {
    float f;
    f = (float)n / 1048576.0;
    f = f * (VIC_PAL & 0x80 ? 1.057777 : 1.066890);
    seta(f);
}
static void ref_ti(void) {
    unsigned char t[4], k;
    unsigned long n;
    CIA2[0x0E] = ((VIC_PAL & 0x80) ^ 0x80) | 0x41;
    CIA2[0x0F] = 0x41;
    while (!(CIA2[4] & 0xC0)) {}
    for (k = 0; k < 4; k++) t[k] = CIA2[4 + k];
    n = 0xFFFFFFFFUL - (t[0] | (unsigned)t[1] << 8 | (unsigned long)t[2] << 16 | (unsigned long)t[3] << 24);
    ref_conv(n);
}
static void ref_ticlr(void) {
    unsigned char k;
    CIA2[0x0E] = ((VIC_PAL & 0x80) ^ 0x80) | 0x40;
    CIA2[0x0F] = 0x40;
    for (k = 0; k < 4; k++) CIA2[4 + k] = 0xFF;
    CIA2[0x0E] |= 0x10;
    CIA2[0x0F] |= 0x10;
    CIA2[0x0E] = ((VIC_PAL & 0x80) ^ 0x80) | 0x41;
    CIA2[0x0F] = 0x41;
}
static void ref_tis(void) {
    unsigned char __far *rtc = (unsigned char __far *)0xFFD7110UL;
    unsigned char b[3], k, i;
    unsigned char *r = _bas_sr;
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
static void ref_raise(int n) { _bas_n = n; _bas_error(); }
static void ref_sleep(void) {
    float wait = geta(), until;
    if (wait < 0.0) ref_raise(14);
    ref_ti();
    until = geta() + wait;
    do ref_ti(); while (geta() < until);
}

/* --- the checks --- */
static unsigned char conv_bad(void) {
    static const unsigned long fixed[] = {
        0, 1, 2, 3, 255, 256, 65535, 65536, 0x7FFFF, 0x80000, 0xFFFFF, 0x100000, 0x100001,
        1000000, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFE, 0xFFFFFFFF,
    };
    unsigned long seed = 12345, n;
    unsigned char bad = 0, i, k, pal = VIC_PAL, x[5];
    for (k = 0; k < 2; k++) {
        if (k) VIC_PAL = pal ^ 0x80;    /* the other standard's scale */
        for (i = 0; i < sizeof fixed / sizeof fixed[0] + 40; i++) {
            if (i < sizeof fixed / sizeof fixed[0]) n = fixed[i];
            else { seed = seed * 1103515245UL + 12345; n = seed >> (i & 15); }
            memcpy(_bas_tcnt, &n, 4);
            memset(_float_a, 0xEE, 5);
            _bas_tconv();
            memcpy(x, _float_a, 5);
            memset(_float_a, 0xEE, 5);
            ref_conv(n);
            if (memcmp(x, _float_a, 5)) bad++;
        }
    }
    VIC_PAL = pal;
    return bad;
}

static unsigned char rbuf[16], sbuf[16];

int main(void) {
    unsigned char bad, i;
    float a, b, c;
    _bas_sr = rbuf;

    OUT[0] = conv_bad();

    /* TI between two of the reference's readings (the timers run) */
    bad = 0;
    for (i = 0; i < 30; i++) {
        ref_ti(); a = geta();
        _bas_ti(); b = geta();
        ref_ti(); c = geta();
        if (a > b || b > c) bad++;
        if (i == 15) _bas_ticlr();
    }
    OUT[1] = bad;

    /* CLR TI: the same control registers; the timers loaded with $FFFF and
     * counting from it (TI small) */
    bad = 0;
    for (i = 0; i < 2; i++) {
        unsigned char ra, rb, t[4];
        if (i) _bas_ticlr(); else ref_ticlr();
        ra = CIA2[0x0E]; rb = CIA2[0x0F];
        t[0] = CIA2[4]; t[1] = CIA2[5]; t[2] = CIA2[6]; t[3] = CIA2[7];
        if (ra != (((VIC_PAL & 0x80) ^ 0x80) | 0x41) || rb != 0x41) bad++;
        if (t[2] != 0xFF || t[3] != 0xFF || t[1] < 0xF0) bad++;
        _bas_ti();
        if (geta() > 0.01) bad++;
    }
    /* stopped timers started by TI */
    CIA2[0x0E] = 0; CIA2[0x0F] = 0;
    _bas_ti();
    if (!(CIA2[0x0E] & 1) || !(CIA2[0x0F] & 1)) bad++;
    OUT[2] = bad;

    /* TI$: when the reference's two readings agree, the same string */
    bad = 1;
    for (i = 0; i < 20; i++) {
        memset(rbuf, 0xEE, sizeof rbuf); _bas_sr = rbuf; ref_tis();
        memset(sbuf, 0xEE, sizeof sbuf); _bas_sr = sbuf; _bas_tis();
        _bas_sr = rbuf;
        {
            unsigned char r2[16];
            memcpy(r2, rbuf, sizeof r2);
            memset(rbuf, 0xEE, sizeof rbuf); ref_tis();
            if (memcmp(r2, rbuf, sizeof r2)) continue;
        }
        bad = memcmp(sbuf, rbuf, sizeof rbuf) != 0;
        if (rbuf[0] != 8 || rbuf[3] != ':' || rbuf[6] != ':') bad = 2;
        break;
    }
    OUT[3] = bad;

    /* SLEEP: at least the wait (and not much more), as the reference's;
     * below 0 ?ILLEGAL QUANTITY (14), 0 none */
    bad = 0;
    for (i = 0; i < 2; i++) {
        _bas_ticlr();
        seta(0.25);
        raised = 0;
        if (!setjmp(back)) { if (i) _bas_sleep(); else ref_sleep(); }
        ref_ti(); a = geta();
        if (raised || a < 0.25 || a > 0.3) bad++;
    }
    seta(-0.5); raised = 0;
    if (!setjmp(back)) _bas_sleep();
    if (raised != 14) bad++;
    seta(-0.5); raised = 0;
    if (!setjmp(back)) ref_sleep();
    if (raised != 14) bad++;
    _bas_ticlr();
    seta(0.0); raised = 0;
    if (!setjmp(back)) _bas_sleep();
    ref_ti();
    if (raised || geta() > 0.01) bad++;
    OUT[4] = bad;

    OUT[5] = 0xAA;
    return 0;
}
