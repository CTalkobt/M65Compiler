/* basic45_ovl/cold.c — cold code in an overlay (test_basic45_ovl_xemu.bas) */
#pragma cc45 overlay(cold)

unsigned int calls;   /* (an overlay's variables stay resident) */

unsigned int cold_add(unsigned char a, unsigned char x) {
    calls++;
    return a + x + 100;
}

unsigned int cold_calls(void) {
    return calls;
}
