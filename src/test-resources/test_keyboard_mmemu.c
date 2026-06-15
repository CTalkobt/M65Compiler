// test_keyboard_mmemu: test_key_pressed() logic and KEY_* constants
// On rawMega65, CIA1 is plain RAM. Pre-load $DC01 with test patterns
// to simulate keyboard matrix responses.
// Results at $4000; run on mmemu rawMega65.
//
// Uses inline C implementation of key_pressed to avoid linking with
// c45.lib (whose DMA-based crt0 hangs in mmemu's DMA handler).

#include <mega65.h>

// Inline reimplementation matching lib/stdlib/key_pressed.s logic
static unsigned char test_key_pressed(unsigned char keycode) {
    static const unsigned char bit_table[] = {1, 2, 4, 8, 16, 32, 64, 128};
    unsigned char col = (keycode >> 3) & 7;
    unsigned char row = keycode & 7;
    *(volatile unsigned char *)0xDC00 = bit_table[col] ^ 0xFF;
    unsigned char val = *(volatile unsigned char *)0xDC01;
    return ((val ^ 0xFF) & bit_table[row]) ? 1 : 0;
}

volatile unsigned char *r = (char *)0x4000;

void main() {
    // --- Test 1: Key constants encode correctly ---
    r[0] = KEY_SPACE;    // col4,row7 = (4<<3)|7 = 0x27
    r[1] = KEY_RETURN;   // col1,row0 = (1<<3)|0 = 0x08
    r[2] = KEY_A;        // col2,row1 = (2<<3)|1 = 0x11
    r[3] = KEY_F1;       // col4,row0 = (4<<3)|0 = 0x20
    r[4] = KEY_STOP;     // col7,row7 = (7<<3)|7 = 0x3F

    // --- Test 2: Simulate "no keys pressed" ---
    // On real hardware, CIA1 PRB reads $FF when no keys pressed (pull-ups).
    // Pre-load $DC01 with $FF to simulate this.
    *(volatile unsigned char *)0xDC01 = 0xFF;

    r[5] = test_key_pressed(KEY_SPACE);   // expect 0 (not pressed)
    r[6] = test_key_pressed(KEY_A);       // expect 0

    // --- Test 3: Simulate SPACE pressed ---
    // SPACE = col 4, row 7. When col 4 is selected ($DC00 = ~$10 = $EF),
    // row 7 bit should be 0 (active-low). So $DC01 = $FF & ~$80 = $7F.
    // But on raw RAM, key_pressed writes to $DC00 (column select) then
    // reads $DC01. We pre-set $DC01 = $7F (row 7 low = space pressed).
    *(volatile unsigned char *)0xDC01 = 0x7F;

    r[7] = test_key_pressed(KEY_SPACE);   // expect 1 (row 7 is low)

    // Reset to no keys
    *(volatile unsigned char *)0xDC01 = 0xFF;
    r[8] = test_key_pressed(KEY_SPACE);   // expect 0 again

    // --- Test 4: Simulate key A pressed ---
    // A = col 2, row 1. Row 1 low: $DC01 = $FF & ~$02 = $FD
    *(volatile unsigned char *)0xDC01 = 0xFD;

    r[9] = test_key_pressed(KEY_A);       // expect 1
    r[10] = test_key_pressed(KEY_SPACE);  // expect 0 (row 7 is still high)

    __asm__("brk");
}
