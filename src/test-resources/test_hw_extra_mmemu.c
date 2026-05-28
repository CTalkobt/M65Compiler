// test_hw_extra_mmemu: DMA controller, math accelerator, audio mixer,
// colour RAM and screen RAM pointer validation.
// Results at $4000; run on mmemu rawMega65.

#include <mega65.h>

volatile unsigned char *r = (char *)0x4000;

void main() {
    // --- Colour RAM and Screen RAM pointers ---
    COLOUR_RAM[0] = 0x01;
    COLOUR_RAM[5] = 0x06;
    SCREEN_RAM[0] = 0x41;
    SCREEN_RAM[3] = 0x44;

    r[0] = *(volatile unsigned char *)0xD800;  // expect 0x01
    r[1] = *(volatile unsigned char *)0xD805;  // expect 0x06
    r[2] = *(volatile unsigned char *)0x0800;  // expect 0x41
    r[3] = *(volatile unsigned char *)0x0803;  // expect 0x44

    // --- DMA controller struct ---
    dma->addr_lo = 0x10;                       // $D701
    dma->addr_mid = 0x20;                      // $D702
    dma->addr_hi = 0x30;                       // $D703
    dma->addr_bank = 0x00;                     // $D704
    dma->etrig = 0xAA;                         // $D705

    r[4] = *(volatile unsigned char *)0xD701;  // expect 0x10
    r[5] = *(volatile unsigned char *)0xD702;  // expect 0x20
    r[6] = *(volatile unsigned char *)0xD703;  // expect 0x30
    r[7] = *(volatile unsigned char *)0xD705;  // expect 0xAA

    // --- Math divider struct ---
    math_div->arg1[0] = 0x64;                 // $D760 (dividend lo = 100)
    math_div->arg1[1] = 0x00;                 // $D761
    math_div->arg2[0] = 0x07;                 // $D764 (divisor lo = 7)
    math_div->sign = 0x01;                    // $D76E

    r[8]  = *(volatile unsigned char *)0xD760; // expect 0x64
    r[9]  = *(volatile unsigned char *)0xD764; // expect 0x07
    r[10] = *(volatile unsigned char *)0xD76E; // expect 0x01

    // --- Math multiplier struct ---
    math_mul->arg1[0] = 0x0A;                 // $D770 (multiplicand = 10)
    math_mul->arg2[0] = 0x05;                 // $D774 (multiplier = 5)

    r[11] = *(volatile unsigned char *)0xD770; // expect 0x0A
    r[12] = *(volatile unsigned char *)0xD774; // expect 0x05

    // --- Audio mixer ---
    audio_mixer->sid1_vol = AUDIO_BOTH(15);   // $D63C = 0xFF
    audio_mixer->sid2_vol = AUDIO_LEFT(10);   // $D63D = 0xA0

    r[13] = *(volatile unsigned char *)0xD63C; // expect 0xFF
    r[14] = *(volatile unsigned char *)0xD63D; // expect 0xA0

    // --- Constants ---
    r[15] = DMA_CMD_COPY;                      // 0x00
    r[16] = DMA_CMD_FILL;                      // 0x03
    r[17] = MATH_BUSY_BIT;                     // 0x80

    __asm__("brk");
}
