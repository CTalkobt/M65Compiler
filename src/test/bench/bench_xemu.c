/* bench_xemu.c — cc45 speed benchmarks on xemu (src/test/bench/run_bench.sh)
 *
 * Each benchmark runs between two stamps of CIA2's timers A and B chained
 * into a 32-bit count down at ~1 MHz (4 bytes each at $C100 + 4*i); its
 * checksum goes to $C0E0 + 2*i, so a wrong result shows. $C0CC = $AA when
 * all ran. -DONLY=n runs benchmark n alone; -DNO_PRINTF skips sprintf.
 */
#include <stdio.h>
#include <string.h>

#define DONE  ((volatile unsigned char *)0xC0CC)
#define CK    ((volatile unsigned char *)0xC0E0)
#define CIA2  ((volatile unsigned char *)0xDD00)
#define STAMP ((volatile unsigned char *)0xC100)

/* CIA2 timers A and B chained: a 32-bit count down of system clocks (~1 MHz) */
static void timer_start(void) {
    CIA2[0x0D] = 0x7F;          /* no NMIs */
    CIA2[0x0E] = 0; CIA2[0x0F] = 0;
    CIA2[0x04] = 0xFF; CIA2[0x05] = 0xFF;
    CIA2[0x06] = 0xFF; CIA2[0x07] = 0xFF;
    CIA2[0x0F] = 0x51;          /* B: count A's underflows, load, start */
    CIA2[0x0E] = 0x11;          /* A: continuous, load, start */
}

static unsigned char nstamp;
static void stamp(void) {
    unsigned char bh, bl, ah, al, i = nstamp * 4;
    do {
        bl = CIA2[0x06]; bh = CIA2[0x07]; ah = CIA2[0x05]; al = CIA2[0x04];
    } while (bl != CIA2[0x06]);
    STAMP[i] = al; STAMP[i + 1] = ah; STAMP[i + 2] = bl; STAMP[i + 3] = bh;
    nstamp++;
}

static void put(int i, unsigned int ck) {
    CK[2 * i] = ck & 0xFF; CK[2 * i + 1] = ck >> 8;
}

/* 0: sieve of Eratosthenes, 8192 flags, 4 times */
static unsigned char flags[8192];

static unsigned int sieve(void) {
    unsigned int i, k, count = 0, iter;
    for (iter = 0; iter < 4; iter++) {
        count = 0;
        for (i = 0; i < 8192; i++) flags[i] = 1;
        for (i = 2; i < 8192; i++) {
            if (flags[i]) {
                for (k = i + i; k < 8192; k += i) flags[k] = 0;
                count++;
            }
        }
    }
    return count;
}

/* 1: recursive calls */
#pragma cc45 recurse
static unsigned int fib(unsigned int n) {
    return n < 2 ? n : fib(n - 1) + fib(n - 2);
}

/* 2: bubble sort of 400 signed ints */
static int arr[400];

static unsigned int bubble(void) {
    int i, j, t;
    unsigned int seed = 12345, ck = 0;
    for (i = 0; i < 400; i++) { seed = seed * 25173 + 13849; arr[i] = (int)(seed >> 3); }
    for (i = 0; i < 399; i++)
        for (j = 0; j < 399 - i; j++)
            if (arr[j] > arr[j + 1]) { t = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = t; }
    for (i = 0; i < 400; i += 37) ck += (unsigned int)arr[i];
    return ck;
}

/* 3: string functions */
static char s1[80], s2[80];

static unsigned int strings(void) {
    unsigned int n, ck = 0;
    for (n = 0; n < 300; n++) {
        strcpy(s1, "The quick brown fox jumps over the lazy dog");
        strcat(s1, " again");
        strcpy(s2, s1);
        s2[n % 40] = 'x';
        ck += strlen(s1) + (strcmp(s1, s2) != 0);
    }
    return ck;
}

/* 4: sprintf */
static char buf[40];

static unsigned int formatting(void) {
    unsigned int n, ck = 0;
#ifdef NO_PRINTF
    return 0;
#endif
    for (n = 0; n < 150; n++) {
        sprintf(buf, "%d:%u:%x", (int)n - 75, n * 7, n * 13);
        ck += strlen(buf) + (unsigned char)buf[1];
    }
    return ck;
}

/* 5: 16- and 32-bit arithmetic, multiply, divide, remainder */
static unsigned int arith(void) {
    unsigned int i, a = 1, b = 7, ck = 0;
    unsigned long l = 1;
    for (i = 1; i < 3000; i++) {
        a = a * 31 + i;
        b = (b ^ a) / (i | 1) + (a % 97);
        l = l * 3 + i;
        ck += a + b + (unsigned int)(l >> 8);
    }
    return ck;
}

#ifndef ONLY
#define ONLY 255
#endif
#define RUN(n, call) do { stamp(); if (ONLY == 255 || ONLY == n) r = call; else r = 0; put(n, r); } while (0)

int main(void) {
    unsigned int r;
    timer_start();
    RUN(0, sieve());
    RUN(1, fib(18));
    RUN(2, bubble());
    RUN(3, strings());
    RUN(4, formatting());
    RUN(5, arith());
    stamp();
    *DONE = 0xAA;
    return 0;
}
