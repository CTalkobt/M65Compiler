/* test_far_xemu.c — research/pcode: sum (pc_sum3) run from bank 0, then
 * from bank 4 and attic RAM (copied there at the same 16-bit address, so its
 * branch targets hold; the bank-0 copy then wiped: a far run reads far), by
 * the far interpreter (pcode_vmf.s45) and translator (pcode_jitf.s45),
 * timed with CIA 2's timer B (interrupts off).
 * $C0C0: [0] native, [1] interpreter (bank 0, 16-bit IP),
 *        [2]-[4] far interpreter: bank 0, bank 4, attic RAM,
 *        [5]-[10] far translator: bank 0 first / again, bank 4 first /
 *        again, attic first / again, [11] the bank-0 copy wiped (it returns
 *        $FFFF now), [12] $AA
 * $C100: the ticks (words) in the same order as [0]-[10] */
#include <dma.h>
#include <string.h>

extern unsigned pcode_run(const unsigned char *code, void *frame);
extern unsigned pcode_runf(unsigned long code, void *frame);
extern unsigned pcode_jitf(unsigned long code, void *frame);
extern void pcode_jitf_flush(void);
extern const unsigned char pc_sum3[];

#define R ((volatile unsigned char *)0xC0C0)
#define T ((volatile unsigned *)0xC100)
#define CIA2_TB_LO (*(volatile unsigned char *)0xDD06)
#define CIA2_TB_HI (*(volatile unsigned char *)0xDD07)
#define CIA2_CRB   (*(volatile unsigned char *)0xDD0F)

#define N 200
#define LEN 30                          /* pc_sum3's bytes */
static unsigned arr[N];
/* (within one page: the translator's frame; 14-15 its element pointer) */
_Alignas(16) static unsigned frame[8];
static unsigned expect;

static native_sum(const unsigned *p, unsigned n) {
    unsigned s = 0;
    for (unsigned i = 0; i < n; i++) s += p[i];
    return s;
}
static void start(void) {
    asm("sei");
    CIA2_CRB = 0;
    CIA2_TB_LO = 0xFF;
    CIA2_TB_HI = 0xFF;
    CIA2_CRB = 0x11;
}
static unsigned stop(void) {
    unsigned char lo, hi;
    CIA2_CRB = 0;
    lo = CIA2_TB_LO;
    hi = CIA2_TB_HI;
    asm("cli");
    return 0xFFFF - ((unsigned)hi << 8 | lo);
}
static void sumFrame(void) { frame[0] = (unsigned)arr; frame[1] = N; }

static unsigned char k;
static void interp(unsigned long at) {
    unsigned r;
    sumFrame();
    start(); r = pcode_runf(at, frame); T[k] = stop();
    R[k++] = r == expect;
}
static void jit(unsigned long at) {
    unsigned r;
    pcode_jitf_flush();
    sumFrame();
    start(); r = pcode_jitf(at, frame); T[k] = stop();
    R[k++] = r == expect;
    sumFrame();
    start(); r = pcode_jitf(at, frame); T[k] = stop();
    R[k++] = r == expect;
}

int main(void) {
    unsigned i, r;
    unsigned long a = (unsigned)pc_sum3, b4 = 0x40000UL + a, at = 0x8000000UL + a;
    for (i = 0; i < N; i++) { arr[i] = i; expect += i; }

    start(); r = native_sum(arr, N); T[0] = stop();
    R[0] = r == expect;
    sumFrame();
    start(); r = pcode_run(pc_sum3, frame); T[1] = stop();
    R[1] = r == expect;
    k = 2;

    dma_copy_far(b4, a, LEN);
    dma_copy_far(at, a, LEN);
    interp(a);                          /* [2] bank 0 */
    k = 5;
    jit(a);                             /* [5] [6] bank 0 (here: before the wipe) */
    memset((void *)pc_sum3, 0, LEN);    /* the bank-0 copy gone */
    sumFrame();
    R[11] = pcode_runf(a, frame) == 0xFFFF;

    k = 3;
    interp(b4);                         /* [3] */
    interp(at);                         /* [4] */
    k = 7;
    jit(b4);                            /* [7] [8] */
    jit(at);                            /* [9] [10] */
    R[12] = 0xAA;
    return 0;
}
