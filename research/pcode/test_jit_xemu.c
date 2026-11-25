/* test_jit_xemu.c — research/pcode: option B (pcode_jit.s45) beside the
 * interpreter (pcode_vm.s45) and native code, on sum (pcode_progs.s45's
 * pc_sum3, the rotated loop), timed with CIA 2's timer B (interrupts off).
 * $C0C0: [0] native 0+1+...+199, [1] interpreter, [2] translated, first run,
 *        [3] translated, again, [4] the chained loop still right for n = 7,
 *        [5] the absolute-address translator (pcode_jit_abs.s45), first run,
 *        [6] again, [7] the peephole made once (LDE + ADD2 in the loop), [8] $AA
 * $C100: the timer's ticks (words): native, interpreter, translated first,
 *        translated again (the frame in the base page), the cache's bytes in
 *        use; absolute-address translated first, again, its cache's bytes */
extern unsigned pcode_run(const unsigned char *code, void *frame);
extern unsigned pcode_jit(const unsigned char *code, void *frame);
extern unsigned pcode_jit_used(void);
extern unsigned char pcode_jit_npeep;
extern unsigned pcode_jita(const unsigned char *code, void *frame);
extern unsigned pcode_jita_used(void);
extern const unsigned char pc_sum3[];

#define R ((volatile unsigned char *)0xC0C0)
#define T ((volatile unsigned *)0xC100)
#define CIA2_TB_LO (*(volatile unsigned char *)0xDD06)
#define CIA2_TB_HI (*(volatile unsigned char *)0xDD07)
#define CIA2_CRB   (*(volatile unsigned char *)0xDD0F)

#define N 200
static unsigned arr[N];
/* (within one page: the base-page translator's frame; 14-15 its element pointer) */
_Alignas(16) static unsigned frame[8];

static unsigned native_sum(const unsigned *p, unsigned n) {
    unsigned s = 0;
    for (unsigned i = 0; i < n; i++) s += p[i];
    return s;
}

static void start(void) {
    asm("sei");
    CIA2_CRB = 0;                     /* stop */
    CIA2_TB_LO = 0xFF;
    CIA2_TB_HI = 0xFF;
    CIA2_CRB = 0x11;                  /* load the latch, run (phi2 ticks) */
}
static unsigned stop(void) {
    unsigned char lo, hi;
    CIA2_CRB = 0;
    lo = CIA2_TB_LO;
    hi = CIA2_TB_HI;
    asm("cli");
    return 0xFFFF - ((unsigned)hi << 8 | lo);
}

static void sumFrame(unsigned n) { frame[0] = (unsigned)arr; frame[1] = n; }

int main(void) {
    unsigned i, r, expect = 0;
    for (i = 0; i < N; i++) { arr[i] = i; expect += i; }

    start(); r = native_sum(arr, N); T[0] = stop();
    R[0] = r == expect;

    sumFrame(N);
    start(); r = pcode_run(pc_sum3, frame); T[1] = stop();
    R[1] = r == expect;

    sumFrame(N);
    start(); r = pcode_jit(pc_sum3, frame); T[2] = stop();
    R[2] = r == expect;

    sumFrame(N);
    start(); r = pcode_jit(pc_sum3, frame); T[3] = stop();
    R[3] = r == expect;

    sumFrame(7);
    R[4] = pcode_jit(pc_sum3, frame) == 21;
    T[4] = pcode_jit_used();

    sumFrame(N);
    start(); r = pcode_jita(pc_sum3, frame); T[5] = stop();
    R[5] = r == expect;
    sumFrame(N);
    start(); r = pcode_jita(pc_sum3, frame); T[6] = stop();
    R[6] = r == expect;
    T[7] = pcode_jita_used();
    R[7] = pcode_jit_npeep == 1;
    R[8] = 0xAA;
    return 0;
}
