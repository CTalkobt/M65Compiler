// cl45 fixture: mega65-isr-rom-call and mega65-sac-interrupt. What an
// __interrupt handler reaches in the file: no ROM calls (not reentrant), no
// static-allocation functions (ln45 refuses them). (noinline: an inlined
// function is not called.)
#include <stdio.h>
#include <cbm.h>

volatile unsigned char ticks;
unsigned char last;

__attribute__((noinline)) static unsigned char twice(unsigned char v) {                   // EXPECT mega65-sac-interrupt
    unsigned char r = v + v;
    return r + ticks;
}

#pragma cc45 recurse
__attribute__((noinline)) static unsigned char framed(unsigned char v) {
    unsigned char r = v + v;
    return r + ticks;
}

__attribute__((noinline)) static void show(unsigned char c) {                             // EXPECT mega65-sac-interrupt
    cbm_k_chrout(c);                                            // EXPECT mega65-isr-rom-call
}

// only the main program calls it
unsigned char report(unsigned char c) {
    putchar(c);
    return twice(c);
}

__interrupt void on_raster(void) {
    ticks++;
    last = twice(ticks) + framed(ticks);
    if (ticks == 50) show('.');
    if (ticks == 100) putchar('!');                             // EXPECT mega65-isr-rom-call
}

// a handler that keeps to itself
__interrupt void on_timer(void) {
    ticks += 2;
}
