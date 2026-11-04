/* main45.c — cc45's harness for the c-bench-64 benchmarks (#129)
 *
 * Replaces the suite's main.c and cia_timer.c: the TOD clock it times with
 * (tenths of a second) does not run in step with a headless xemu, so the
 * time here is CIA 2's timer A (phi2 ticks, about 1 MHz at every CPU speed)
 * counted into timer B: 32 bits of ticks.
 *
 * BENCH_SPEED: 1 (the C64's speed, as the published results), 3 (3.5 MHz),
 * or 40 (the MEGA65's full speed), set before the benchmark runs.
 *
 * Results at $C0C0 (run_bench.py reads them from xemu's memory dump):
 *   [0]    benchmark_check(): 0 pass, 1 fail
 *   [1..4] the ticks, low byte first
 *   [5]    $AA sentinel
 * and what the benchmark printed at $C100 (out45.c). The screen shows one
 * line: the check's result and the ticks (hex).
 */
#include <stdio.h>
#include "out.h"

#ifndef BENCH_SPEED
#define BENCH_SPEED 1
#endif

#define POKE(a, v) (*(volatile unsigned char *)(a) = (v))
#define PEEK(a) (*(volatile unsigned char *)(a))
#define RESULT ((volatile unsigned char *)0xC0C0)

void benchmark(void);
void benchmark_name(void);
unsigned char benchmark_check(void);

/* the suite's cia_timer.h: some benchmarks call these */
void tod_reset(void) { POKE(0xDD0B, 0); POKE(0xDD0A, 0); POKE(0xDD09, 0); POKE(0xDD08, 0); }
unsigned int tod_get10(void) { return 0; }
void tod_init(unsigned char freq) { (void)freq; }
unsigned char tod_freq(void) { return 50; }

static void say(const char *s) { while (*s) putchar(*s++); }
static void hex2(unsigned char b) {
    putchar("0123456789ABCDEF"[b >> 4]);
    putchar("0123456789ABCDEF"[b & 15]);
}

static void set_speed(void) {
    unsigned char d031 = PEEK(0xD031) & 0xBF, d054 = PEEK(0xD054) & 0xBF;
#if BENCH_SPEED == 40
    d031 |= 0x40;
    d054 |= 0x40;
#elif BENCH_SPEED == 3
    d031 |= 0x40;
#endif
    POKE(0xD031, d031);
    POKE(0xD054, d054);
}

int main(void) {
    unsigned char check;
    unsigned char t0, t1, t2, t3;

    benchmark_name();
    set_speed();
    POKE(0xDD0E, 0x00);                 /* timers stopped */
    POKE(0xDD0F, 0x00);
    POKE(0xDD04, 0xFF); POKE(0xDD05, 0xFF);
    POKE(0xDD06, 0xFF); POKE(0xDD07, 0xFF);
    POKE(0xDD0F, 0x51);                 /* B: load, counts A's underflows, start */
    POKE(0xDD0E, 0x11);                 /* A: load, counts phi2, start */
    benchmark();
    POKE(0xDD0E, 0x00);
    POKE(0xDD0F, 0x40);
    t0 = PEEK(0xDD04); t1 = PEEK(0xDD05); t2 = PEEK(0xDD06); t3 = PEEK(0xDD07);
    print(NEWLINE);
    check = benchmark_check();

    RESULT[0] = check;
    RESULT[1] = ~t0;
    RESULT[2] = ~t1;
    RESULT[3] = ~t2;
    RESULT[4] = ~t3;
    RESULT[5] = 0xAA;
    /* (putchar, not printf: printf would add 2 KB to the program's size) */
    say(check ? "CHECK 1, $" : "CHECK 0, $");   /* (the KERNAL readers: 1 on a MEGA65) */
    hex2(~t3); hex2(~t2); hex2(~t1); hex2(~t0);   /* ($C0C0 reads as ROM here) */
    say(" TICKS\n");
    return 0;
}
