#pragma cc45 no_zp_save
/* test_float_xemu.c — Validate float.h functions via xemu
 *
 * Tests basic float arithmetic and math functions using CBM 40-bit ROM routines.
 * Uses global float variables to avoid ZP allocation conflicts.
 *
 * KNOWN ISSUE: (int)float cast doesn't call __float_ftoi — compiler generates
 * inline truncation that reads wrong bytes. Tests 1-4 and 6-7 will fail until
 * codegen emits proper __float_ftoi call for F32→I16 casts.
 * Test 5 (comparison) works because __float_cmp is correctly called.
 *
 * Expected at $C090: 01 02 03 04 05 06 07 AA
 */
/* No math.h — fabsf/sinf etc. have naming mismatch with ROM wrappers */

#define RESULT(i) (*(volatile unsigned char *)(0xC090 + (i)))

/* Global float variables — avoids ZP lifetime issues */
float ga, gb, gc;

void main() {
    /* Test 1: float addition — 3.0 + 2.0 = 5.0 */
    ga = 3.0;
    gb = 2.0;
    gc = ga + gb;
    if ((int)gc == 5) RESULT(0) = 0x01;

    /* Test 2: float subtraction — 10.0 - 3.0 = 7.0 */
    ga = 10.0;
    gb = 3.0;
    gc = ga - gb;
    if ((int)gc == 7) RESULT(1) = 0x02;

    /* Test 3: float multiplication — 4.0 * 3.0 = 12.0 */
    ga = 4.0;
    gb = 3.0;
    gc = ga * gb;
    if ((int)gc == 12) RESULT(2) = 0x03;

    /* Test 4: float division — 15.0 / 3.0 = 5.0 */
    ga = 15.0;
    gb = 3.0;
    gc = ga / gb;
    if ((int)gc == 5) RESULT(3) = 0x04;

    /* Test 5: float comparison */
    ga = 3.14;
    gb = 3.14;
    gc = 2.71;
    if (ga == gb && ga != gc) RESULT(4) = 0x05;

    /* Test 6: int-to-float-to-int roundtrip */
    ga = (float)42;
    if ((int)ga == 42) RESULT(5) = 0x06;

    /* Test 7: negative float to int */
    ga = -5.0;
    gb = 0.0 - ga;  /* negate manually */
    if ((int)gb == 5) RESULT(6) = 0x07;

    RESULT(7) = 0xAA;
}
