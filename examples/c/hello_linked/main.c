#include <stdio.h>

int main() {
    /* Test: write 42 to MULTINA, 10 to MULTINB, read quotient from DIVOUT */

    /* Unlock MEGA65 I/O */
    asm("lda #$47");
    asm("sta $d02f");
    asm("lda #$53");
    asm("sta $d02f");

    puts("io unlocked");

    /* Write MULTINA = 42 ($D770-$D773) */
    asm("lda #42");
    asm("sta $d770");
    asm("lda #0");
    asm("sta $d771");
    asm("sta $d772");
    asm("sta $d773");

    /* Write MULTINB = 10 ($D774-$D777) */
    asm("lda #10");
    asm("sta $d774");
    asm("lda #0");
    asm("sta $d775");
    asm("sta $d776");
    asm("sta $d777");

    puts("regs written");

    /* Wait for divider */
    asm("@wdiv: bit $d70f");
    asm("bne @wdiv");

    puts("div done");

    /* Read quotient from DIVOUT whole ($D76C) */
    /* If 42/10 = 4, then $D76C should be 4 */
    asm("lda $d76c");
    asm("clc");
    asm("adc #$30");   /* convert to ASCII digit */
    asm("sta $0800");   /* write to screen RAM */

    puts("check screen");
    return 0;
}
