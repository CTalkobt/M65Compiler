/* test_isr_page_xemu.c — __interrupt handlers run on their own base page
 *
 * The handler is entered as an interrupt would enter it (return address
 * and P pushed, then a jump; its RTI returns), from the middle of mainline
 * code whose zero-page temporaries must survive.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] handler's B is __zp_page_isr, not the program's page
 *   [1] after a ROM call (putchar) the handler is back on its own page
 *   [2] __isr_depth is 0 again after the handler
 *   [3] mainline zero-page values survive the handler
 *   [4] the handler's own work (a call with ZP parameters) is right
 *   [5] mainline B is restored after the handler
 *   [6] $AA sentinel
 */
#include <stdio.h>

#define OUT ((volatile unsigned char *)0xC0C0)

extern unsigned char _isr_depth;   /* crt0's __isr_depth */
unsigned char b_isr, b_isr_after_rom, b_main, b_main_after, isr_sum;

/* Called from the handler and from main: a stack frame, not SAC storage */
#pragma cc45 recurse
int add3(int a, int b, int c) { return a + b + c; }

__interrupt void handler(void) {
    __asm__("tba");
    __asm__("sta _b_isr");
    putchar('I');
    __asm__("tba");
    __asm__("sta _b_isr_after_rom");
    isr_sum = add3(10, 20, 30);
}

static void raise(void) {
    __asm__("lda #>@ret");
    __asm__("pha");
    __asm__("lda #<@ret");
    __asm__("pha");
    __asm__("php");
    __asm__("jmp _handler");
    __asm__("@ret:");
}

int main(void) {
    int a = add3(1, 2, 3), b = add3(4, 5, 6);
    __asm__("tba");
    __asm__("sta _b_main");
    raise();
    __asm__("tba");
    __asm__("sta _b_main_after");
    OUT[0] = b_isr != b_main && b_isr != 0;
    OUT[1] = b_isr_after_rom == b_isr;
    OUT[2] = _isr_depth == 0;
    OUT[3] = a + add3(b, 0, 0) == 21;
    OUT[4] = isr_sum == 60;
    OUT[5] = b_main_after == b_main;
    OUT[6] = 0xAA;
    return 0;
}
