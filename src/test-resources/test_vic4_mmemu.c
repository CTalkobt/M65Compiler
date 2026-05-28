// test_vic4_mmemu: VIC-IV struct overlay and VREG direct access
// Validates struct offsets, VREG defines, and 8-bit store width.
// Results at $4000; run on mmemu rawMega65.

#include <mega65.h>

volatile unsigned char *r = (char *)0x4000;

void main() {
    // --- Test 1: Struct writes land at correct VIC-IV addresses ---
    vic4->border = 0xAA;       // $D020
    vic4->bg0 = 0xBB;          // $D021
    vic4->bg1 = 0xCC;          // $D022
    vic4->bg2 = 0xDD;          // $D023
    vic4->sprite_enable = 0x0F; // $D015

    // Read back from hardware addresses into result buffer
    r[0] = *(volatile unsigned char *)0xD020;  // expect 0xAA
    r[1] = *(volatile unsigned char *)0xD021;  // expect 0xBB
    r[2] = *(volatile unsigned char *)0xD022;  // expect 0xCC
    r[3] = *(volatile unsigned char *)0xD023;  // expect 0xDD
    r[4] = *(volatile unsigned char *)0xD015;  // expect 0x0F

    // --- Test 2: VREG direct defines ---
    VREG_BORDER = 0x11;        // $D020
    VREG_BG0 = 0x22;           // $D021

    r[5] = *(volatile unsigned char *)0xD020;  // expect 0x11
    r[6] = *(volatile unsigned char *)0xD021;  // expect 0x22

    // --- Test 3: 8-bit store width (no adjacent clobber) ---
    // Write known pattern, then overwrite middle byte only
    VREG_BORDER = 0xE1;        // $D020
    VREG_BG0 = 0xE2;           // $D021
    VREG_BG1 = 0xE3;           // $D022

    // Now overwrite only bg0
    VREG_BG0 = 0xFF;           // should only touch $D021

    r[7] = *(volatile unsigned char *)0xD020;  // expect 0xE1 (untouched)
    r[8] = *(volatile unsigned char *)0xD021;  // expect 0xFF (overwritten)
    r[9] = *(volatile unsigned char *)0xD022;  // expect 0xE3 (untouched)

    // --- Test 4: Struct write 8-bit width ---
    vic4->border = 0xC1;
    vic4->bg0 = 0xC2;
    vic4->bg1 = 0xC3;

    // Overwrite only bg0 via struct
    vic4->bg0 = 0x99;

    r[10] = *(volatile unsigned char *)0xD020; // expect 0xC1 (untouched)
    r[11] = *(volatile unsigned char *)0xD021; // expect 0x99 (overwritten)
    r[12] = *(volatile unsigned char *)0xD022; // expect 0xC3 (untouched)

    // --- Test 5: Colour and bit constants ---
    r[13] = COLOR_BLACK;   // 0x00
    r[14] = COLOR_BLUE;    // 0x06
    r[15] = COLOR_LGREY;   // 0x0F
    r[16] = VIC4_DEN;      // 0x10
    r[17] = VIC4_FAST;     // 0x40
    r[18] = VIC4_FCM;      // 0x04

    __asm__("brk");
}
